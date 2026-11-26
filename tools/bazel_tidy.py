#!/usr/bin/env python3
"""Keep local Bazel disk use bounded. build.sh runs this at most once a day.

1. Trims the shared disk cache: if it is over its limit, removes entries older
   than 14 days, then the oldest entries until it is under the limit. This is
   the policy of the CI gc-bazel-disk-cache step.
2. Removes stale output bases under the output user root: those whose checkout
   no longer exists, and those unused for AL_BAZEL_TIDY_MAX_AGE_DAYS (30) days.
3. If all output bases together are still over AL_BAZEL_OUTPUT_BUDGET_GB (100),
   removes the least recently used until under budget.

The current checkout's output base and any output base with a running Bazel
server are never removed. Errors are printed and ignored; this script never
fails the build. Every output base under the output user root counts, not only
this repository's.
"""
import argparse
import os
import shutil
import stat
import sys
import time

GB = 1024 ** 3
DAY = 24 * 3600
CACHE_ENTRY_MAX_AGE_DAYS = 14


def env_number(name, default):
    try:
        return float(os.environ.get(name, default))
    except ValueError:
        return float(default)


def tree_size(path):
    total = 0
    for root, dirs, files in os.walk(path, onerror=lambda e: None):
        for name in files:
            try:
                total += os.lstat(os.path.join(root, name)).st_size
            except OSError:
                pass
    return total


def fmt_gb(size):
    return "%.1f GB" % (size / GB)


def make_writable(path):
    for root, dirs, files in os.walk(path, onerror=lambda e: None):
        for name in dirs + files:
            full = os.path.join(root, name)
            try:
                mode = os.lstat(full).st_mode
                if not stat.S_ISLNK(mode):
                    os.chmod(full, mode | stat.S_IWUSR | (stat.S_IXUSR if stat.S_ISDIR(mode) else 0))
            except OSError:
                pass


def remove_tree(path):
    try:
        os.chmod(path, os.lstat(path).st_mode | stat.S_IWUSR | stat.S_IXUSR)
    except OSError:
        pass
    make_writable(path)
    shutil.rmtree(path, ignore_errors=True)


def trim_disk_cache(cache, max_bytes, dry_run):
    if not os.path.isdir(cache):
        return
    before = tree_size(cache)
    if before <= max_bytes:
        return
    entries = []
    for sub in ("cas", "ac"):
        for root, dirs, files in os.walk(os.path.join(cache, sub), onerror=lambda e: None):
            for name in files:
                full = os.path.join(root, name)
                try:
                    st = os.lstat(full)
                except OSError:
                    continue
                entries.append((st.st_mtime, st.st_size, full))
    entries.sort()
    cutoff = time.time() - CACHE_ENTRY_MAX_AGE_DAYS * DAY
    used = before
    removed = 0
    for mtime, size, full in entries:
        if used <= max_bytes and mtime >= cutoff:
            break
        if not dry_run:
            try:
                os.unlink(full)
            except OSError:
                continue
        used -= size
        removed += 1
    verb = "would remove" if dry_run else "removed"
    print("bazel disk cache: %s -> %s (limit %s); %s %d entries"
          % (fmt_gb(before), fmt_gb(used), fmt_gb(max_bytes), verb, removed))


def server_running(base):
    try:
        with open(os.path.join(base, "server", "server.pid.txt")) as f:
            pid = int(f.read().strip())
    except (OSError, ValueError):
        return False
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def last_used(base):
    times = []
    for name in ("command.log", "server"):
        try:
            times.append(os.stat(os.path.join(base, name)).st_mtime)
        except OSError:
            pass
    if not times:
        try:
            times.append(os.stat(base).st_mtime)
        except OSError:
            times.append(0)
    return max(times)


def output_bases(root):
    found = []
    try:
        names = os.listdir(root)
    except OSError:
        return found
    for name in names:
        base = os.path.join(root, name)
        marker = os.path.join(base, "DO_NOT_BUILD_HERE")
        if not os.path.isfile(marker):
            continue
        try:
            with open(marker) as f:
                checkout = f.read().strip()
        except OSError:
            checkout = ""
        found.append({"path": base, "checkout": checkout, "last_used": last_used(base)})
    return found


def tidy_output_bases(root, current, max_age_days, budget_bytes, dry_run):
    bases = output_bases(root)
    if not bases:
        return
    current = os.path.realpath(current) if current else ""
    now = time.time()
    verb = "would remove" if dry_run else "removed"

    def protected(b):
        return os.path.realpath(b["path"]) == current or server_running(b["path"])

    def label(b):
        return b["checkout"] or os.path.basename(b["path"])

    stale = []
    for b in bases:
        if protected(b):
            continue
        if b["checkout"] and not os.path.isdir(b["checkout"]):
            stale.append((b, "checkout gone"))
        elif now - b["last_used"] > max_age_days * DAY:
            stale.append((b, "unused %d days" % ((now - b["last_used"]) // DAY)))
    if stale:
        freed = 0
        notes = []
        for b, why in stale:
            size = tree_size(b["path"])
            if not dry_run:
                remove_tree(b["path"])
            freed += size
            notes.append("%s (%s)" % (label(b), why))
        print("bazel output bases: %s %d stale (%s): %s"
              % (verb, len(stale), fmt_gb(freed), ", ".join(notes)))

    removed_paths = {b["path"] for b, _ in stale}
    remaining = [b for b in bases if b["path"] not in removed_paths]
    sizes = {b["path"]: tree_size(b["path"]) for b in remaining}
    total = sum(sizes.values())
    if total <= budget_bytes:
        return
    before = total
    evicted = []
    for b in sorted(remaining, key=lambda b: b["last_used"]):
        if total <= budget_bytes:
            break
        if protected(b):
            continue
        if not dry_run:
            remove_tree(b["path"])
        total -= sizes[b["path"]]
        evicted.append(label(b))
    line = "bazel output bases: %s -> %s (budget %s)" % (fmt_gb(before), fmt_gb(total), fmt_gb(budget_bytes))
    if evicted:
        line += "; %s %d least recently used: %s" % (verb, len(evicted), ", ".join(evicted))
    if total > budget_bytes:
        line += "; still over budget, running `bazel clean` in large checkouts would help"
    print(line)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output-user-root", required=True)
    parser.add_argument("--current-output-base", default="")
    parser.add_argument("--disk-cache", default=os.path.expanduser("~/.cache/bazel-disk-cache"))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    dry_run = args.dry_run or os.environ.get("AL_BAZEL_TIDY") == "dry"
    max_cache = env_number("AL_BAZEL_DISK_CACHE_MAX", 30) * GB
    max_age = env_number("AL_BAZEL_TIDY_MAX_AGE_DAYS", 30)
    budget = env_number("AL_BAZEL_OUTPUT_BUDGET_GB", 100) * GB

    for step in (lambda: trim_disk_cache(args.disk_cache, max_cache, dry_run),
                 lambda: tidy_output_bases(args.output_user_root, args.current_output_base,
                                           max_age, budget, dry_run)):
        try:
            step()
        except Exception as e:  # never fail the build
            print("bazel tidy: %s" % e, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
