#!/usr/bin/env python3

import subprocess
import re


def run_and_parse_output_from_bench_cmd(cmd, runtime):
    output = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT)
    all_lines = output.decode().splitlines()

    stats = []

    lines = []
    for line in all_lines:
        # Skip a log line printed by go compilation that contains 'Using'
        if "in statically linked applications requires at runtime" in line:
            continue

        if "Using" in line or "Inserted" in line or "Looked up" in line:
            lines.append(line)

    for i in range(len(lines) // 3):
        # Extract a label
        label = lines[3 * i].partition("Using ")[2].replace('"', "")

        # Extract what is inside the parenthesis
        # time="2025-09-29T12:02:36-07:00" level=info msg="Inserted 1,000,000 keys in 0.072s (13,901,028 keys/sec)\n"
        inserts_qps = lines[3 * i + 1]
        match = re.search(r"\((.*?)\)", inserts_qps)
        if match:
            inside = match.group(1)
            inserts = inside.split()[0]

        lookups_qps = lines[3 * i + 2]
        match = re.search(r"\((.*?)\)", lookups_qps)
        if match:
            inside = match.group(1)
            lookups = inside.split()[0]

        stats.append((label, inserts, lookups, runtime))

    return stats


if __name__ == "__main__":
    cpp_cmd = "./build/main/axoncache_cli --bench"
    go_cmd = "go run cmd/benchmark.go cmd/kv_scanner.go cmd/main.go cmd/progress.go benchmark"
    python_cmd = "python3 axoncache/bench.py"

    labels = """\
| [C++ unordered_map](https://github.com/AppLovin/AxonCache)               |
| [Abseil flat_map](https://abseil.io/docs/cpp/guides/container)           |
| [AxonCache](https://github.com/AppLovin/AxonCache) C api                 |
| [Go Map](https://pkg.go.dev/builtin#map)                                 |
| [AxonCache](https://github.com/AppLovin/AxonCache) Golang                |
| [LMDB](https://symas.com/lmdb/)                                          |
| [LevelDB](https://github.com/syndtr/goleveldb) Pure Go version           |
| [CDB](https://cr.yp.to/cdb.html) Pure Go Version with mmap support       |
| [AxonCache](https://github.com/AppLovin/AxonCache) Python                |
| [LMDB](https://github.com/jnwatson/py-lmdb/) Python module               |
| [CDB](https://github.com/bbayles/python-pure-cdb) Pure Python module     |
""".splitlines()

    cmds = [
        (cpp_cmd, "C++"),
        (go_cmd, "Golang"),
        (python_cmd, "Python"),
    ]

    stats = []
    for cmd, runtime in cmds:
        runtime_stats = run_and_parse_output_from_bench_cmd(cmd, runtime)
        stats.extend(runtime_stats)

    for stat, label in zip(stats, labels):
        print(
            "{} {:<18} | {:<15} | {:7} |".format(
                label,
                stat[1],
                stat[2],
                stat[3],  # Display stat[0] to make sure everything line up
            )
        )
