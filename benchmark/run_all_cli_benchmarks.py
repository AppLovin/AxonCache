#!/usr/bin/env python3
import subprocess
import re
from collections import defaultdict
from statistics import mean, stdev


# ---------- Helpers ----------
def latency_ns_from_qps(qps: float) -> float:
    if qps <= 0:
        return float("inf")
    return 1e9 / qps


def parse_qps(text: str) -> int:
    return int(text.replace(",", ""))


def fmt_avg_std(avg: float, sd: float, unit: str = "", width: int = 28) -> str:
    s = (
        f"{avg:,.1f}{unit}"
        if not (sd and sd == sd)
        else f"{avg:,.1f}{unit} ± {sd:,.1f}{unit}"
    )
    return f"{s:<{width}}"


# ---------- Run + parse ----------
def run_and_parse_output_from_bench_cmd(cmd, runtime):
    """
    Returns list of tuples:
      (raw_label, inserts_qps:int, lookups_qps:int, runtime:str)
    """
    output = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT)
    all_lines = output.decode().splitlines()

    lines = []
    for line in all_lines:
        # Skip a noisy Go static link warning
        if "in statically linked applications requires at runtime" in line:
            continue
        if "Using" in line or "Inserted" in line or "Looked up" in line:
            lines.append(line)

    stats = []
    # Expect groups of 3 lines: Using..., Inserted..., Looked up...
    for i in range(len(lines) // 3):
        raw_label = lines[3 * i].partition("Using ")[2].replace('"', "").strip()

        inserts_qps_line = lines[3 * i + 1]
        m = re.search(r"\((.*?)\)", inserts_qps_line)
        if not m:
            continue
        inserts_qps = parse_qps(m.group(1).split()[0])

        lookups_qps_line = lines[3 * i + 2]
        m = re.search(r"\((.*?)\)", lookups_qps_line)
        if not m:
            continue
        lookups_qps = parse_qps(m.group(1).split()[0])

        stats.append((raw_label, inserts_qps, lookups_qps, runtime))
    return stats


# ---------- Raw → Pretty label mapping (per runtime) ----------
RAW_TO_PRETTY = {
    (
        "C++",
        "Unordered map",
    ): "[unordered_map](https://cppreference.net/cpp/container/unordered_map.html)",
    (
        "C++",
        "Abseil flat map",
    ): "[Abseil flat_map](https://abseil.io/docs/cpp/guides/container)",
    ("C++", "AxonCache"): "[AxonCache](https://github.com/AppLovin/AxonCache) C api",
    ("Golang", "Native go maps"): "[Go Map](https://pkg.go.dev/builtin#map)",
    (
        "Golang",
        "AxonCache",
    ): "[AxonCache](https://github.com/AppLovin/AxonCache) Golang",
    ("Golang", "LMDB"): "[LMDB](https://symas.com/lmdb/)",
    (
        "Golang",
        "LevelDB",
    ): "[LevelDB](https://github.com/syndtr/goleveldb) Pure Go version",
    (
        "Golang",
        "CDB",
    ): "[CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support",
    (
        "Python",
        "AxonCache",
    ): "[AxonCache](https://github.com/AppLovin/AxonCache) Python",
    ("Python", "LMDB"): "[LMDB](https://github.com/jnwatson/py-lmdb/) Python module",
    (
        "Python",
        "Python CDB",
    ): "[CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module",
    (
        "Java",
        "HashMap",
    ): "[HashMap](https://www.baeldung.com/java-hashmap) Java HashMap",
    (
        "Java",
        "AxonCache",
    ): "[AxonCache](https://github.com/AppLovin/AxonCache) Java",
}

# ---------- Main ----------
if __name__ == "__main__":
    cpp_cmd = "./build/main/axoncache_cli --bench"
    go_cmd = "go run cmd/benchmark.go cmd/kv_scanner.go cmd/main.go cmd/progress.go benchmark"
    python_cmd = "python3 axoncache/bench.py"
    java_cmd = "sh java/run_benchmark.sh"

    cmds = [
        (cpp_cmd, "C++"),
        (go_cmd, "Golang"),
        (python_cmd, "Python"),
        (java_cmd, "Java"),
    ]

    runs = 3

    # aggregates[pretty_label] = lists + runtime + first-seen execution order
    aggregates = defaultdict(
        lambda: {
            "runtime": None,
            "inserts_qps": [],
            "lookups_qps": [],
            "first_seen": None,  # order index when first encountered
        }
    )

    order_counter = 0  # increases in the exact order rows are parsed/executed

    total_iterations = len(RAW_TO_PRETTY) * runs
    idx = 1

    # Collect
    for _ in range(runs):
        for cmd, runtime in cmds:
            for raw_label, ins_qps, look_qps, rt in run_and_parse_output_from_bench_cmd(
                cmd, runtime
            ):
                print("Step %d out of %d" % (idx, total_iterations))
                idx += 1

                pretty = RAW_TO_PRETTY.get((rt, raw_label))
                if not pretty:
                    # Unmapped: skip (or print a warning if you prefer)
                    print(f"Unmapped label seen: ({rt}, {raw_label})")
                    continue
                a = aggregates[pretty]
                a["runtime"] = rt
                a["inserts_qps"].append(ins_qps)
                a["lookups_qps"].append(look_qps)
                if a["first_seen"] is None:
                    a["first_seen"] = order_counter
                order_counter += 1

    # Build rows (compute stats)
    rows = []
    for pretty, vals in aggregates.items():
        ins = vals["inserts_qps"]
        look = vals["lookups_qps"]
        if not ins or not look:
            continue

        ins_avg, look_avg = mean(ins), mean(look)
        ins_sd = stdev(ins) if len(ins) >= 2 else 0.0
        look_sd = stdev(look) if len(look) >= 2 else 0.0

        ins_lat = [latency_ns_from_qps(q) for q in ins]
        look_lat = [latency_ns_from_qps(q) for q in look]
        ins_lat_avg, look_lat_avg = mean(ins_lat), mean(look_lat)
        ins_lat_sd = stdev(ins_lat) if len(ins_lat) >= 2 else 0.0
        look_lat_sd = stdev(look_lat) if len(look_lat) >= 2 else 0.0

        rows.append(
            {
                "label": pretty,
                "runtime": vals["runtime"],
                "first_seen": vals["first_seen"],
                "look_lat_avg": look_lat_avg,
                "look_lat_sd": look_lat_sd,
                "look_qps_avg": look_avg,
                "look_qps_sd": look_sd,
                "ins_lat_avg": ins_lat_avg,
                "ins_lat_sd": ins_lat_sd,
                "ins_qps_avg": ins_avg,
                "ins_qps_sd": ins_sd,
            }
        )

    # ---- Sort by lookup latency (ascending, fastest first)
    # Break ties by first-seen execution order to keep a stable, execution-respecting order.
    rows.sort(key=lambda r: (r["look_lat_avg"], r["first_seen"]))

    # ---- Print aligned Markdown table
    header = "| {:<76} | {:<7} | {:<28} | {:<28} | {:<28} | {:<28} |".format(
        "Implementation",
        "Runtime",
        "Lookups Lat (ns ± sd)",
        "Lookups QPS (avg ± sd)",
        "Inserts Lat (ns ± sd)",
        "Inserts QPS (avg ± sd)",
    )
    divider = (
        "|"
        + "-" * 78
        + "|"
        + "-" * 9
        + "|"
        + "-" * 30
        + "|"
        + "-" * 30
        + "|"
        + "-" * 30
        + "|"
        + "-" * 30
        + "|"
    )
    print(header)
    print(divider)

    for r in rows:
        print(
            "| {:<76} | {:<7} | {:<28} | {:<28} | {:<28} | {:<28} |".format(
                r["label"],
                r["runtime"],
                fmt_avg_std(r["look_lat_avg"], r["look_lat_sd"], " ns", 28).strip(),
                fmt_avg_std(r["look_qps_avg"], r["look_qps_sd"], "", 28).strip(),
                fmt_avg_std(r["ins_lat_avg"], r["ins_lat_sd"], " ns", 28).strip(),
                fmt_avg_std(r["ins_qps_avg"], r["ins_qps_sd"], "", 28).strip(),
            )
        )
