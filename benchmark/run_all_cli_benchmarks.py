#!/usr/bin/env python3

import subprocess
import re


def run_and_parse_output_from_bench_cmd(cmd, runtime):
    output = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT)
    all_lines = output.decode().splitlines()

    stats = []

    lines = []
    for line in all_lines:
        if 'Using' in line or 'Inserted' in line or 'Looked up' in line:
            lines.append(line)

    for i in range(len(lines) // 3):
        # Extract a label
        label = lines[3 * i].partition("Using ")[2].replace("\"", '')

        # Extract what is inside the parenthesis
        # time="2025-09-29T12:02:36-07:00" level=info msg="Inserted 1,000,000 keys in 0.072s (13,901,028 keys/sec)\n"
        inserts_qps = lines[3 * i + 1]
        match = re.search(r'\((.*?)\)', inserts_qps)
        if match:
            inside = match.group(1)
            inserts = inside.split()[0]

        lookups_qps = lines[3 * i + 2]
        match = re.search(r'\((.*?)\)', lookups_qps)
        if match:
            inside = match.group(1)
            lookups = inside.split()[0]

        stats.append((label, inserts, lookups, runtime))

    return stats


if __name__ == '__main__':
    cpp_cmd = './build/main/axoncache_cli --bench'
    go_cmd = 'go run cmd/benchmark.go cmd/kv_scanner.go cmd/main.go cmd/progress.go benchmark'
    python_cmd = 'python3 axoncache/bench.py'

    urls = {
        "axon_cache": ("https://github.com/AppLovin/AxonCache", "noop"),
        "flat_map": ("https://abseil.io/docs/cpp/guides/container", "blah"),
        "go_map": ("https://pkg.go.dev/builtin#map", "blob"),
        "cdb": ("https://cr.yp.to/cdb.html", "aadsc"),
        "LMDB": ("https://symas.com/lmdb/", "asdc"),
        "LevelDB": ("https://github.com/syndtr/goleveldb", "Pure Go version"),
        "LMDB": ("https://github.com/jnwatson/py-lmdb/", "Python module"),
        "CDB": ("https://github.com/bbayles/python-pure-cdb", "CDB"),
    }

    cmds = [
        (cpp_cmd, 'C++'),
        (go_cmd, 'Golang'),
        (python_cmd, 'Python'),
     ]

    stats = []
    for cmd, runtime in cmds:
        runtime_stats = run_and_parse_output_from_bench_cmd(cmd, runtime)
        stats.extend(runtime_stats)

    for stat in stats:
        print(stat)

