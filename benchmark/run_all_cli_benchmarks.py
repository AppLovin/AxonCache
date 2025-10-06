#!/usr/bin/env python3
import subprocess
import re
from collections import defaultdict
from statistics import mean, stdev

def latency_ns_from_qps(qps: float) -> float:
    if qps <= 0:
        return float("inf")
    return 1e9 / qps

def parse_qps(text: str) -> int:
    return int(text.replace(",", ""))

def run_and_parse_output_from_bench_cmd(cmd, runtime):
    """
    Returns list of tuples:
      (label, inserts_qps:int, lookups_qps:int, runtime:str)
    """
    output = subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT)
    all_lines = output.decode().splitlines()

    stats = []
    lines = []
    for line in all_lines:
        # Skip noisy Go static link warning
        if "in statically linked applications requires at runtime" in line:
            continue
        if "Using" in line or "Inserted" in line or "Looked up" in line:
            lines.append(line)

    for i in range(len(lines) // 3):
        label = lines[3 * i].partition("Using ")[2].replace('"', "").strip()

        # Inserted ... (XYZ keys/sec)
        inserts_qps_line = lines[3 * i + 1]
        m = re.search(r"\((.*?)\)", inserts_qps_line)
        if not m:
            continue
        inside = m.group(1)
        inserts_qps = parse_qps(inside.split()[0])

        # Looked up ... (XYZ keys/sec)
        lookups_qps_line = lines[3 * i + 2]
        m = re.search(r"\((.*?)\)", lookups_qps_line)
        if not m:
            continue
        inside = m.group(1)
        lookups_qps = parse_qps(inside.split()[0])

        stats.append((label, inserts_qps, lookups_qps, runtime))

    return stats

def fmt_avg_std(avg: float, sd: float, unit: str = "", width: int = 22) -> str:
    """Formats avg ± sd with alignment and optional units."""
    if sd == 0 or sd != sd:  # NaN
        s = f"{avg:,.1f}{unit}"
    else:
        s = f"{avg:,.1f}{unit} ± {sd:,.1f}{unit}"
    return f"{s:<{width}}"

if __name__ == "__main__":
    cpp_cmd = "./build/main/axoncache_cli --bench"
    go_cmd = "go run cmd/benchmark.go cmd/kv_scanner.go cmd/main.go cmd/progress.go benchmark"
    python_cmd = "python3 axoncache/bench.py"

    cmds = [
        (cpp_cmd, "C++"),
        (go_cmd, "Golang"),
        (python_cmd, "Python"),
    ]

    runs = 3
    aggregates = defaultdict(lambda: {"inserts": [], "lookups": [], "runtime": None})

    for _ in range(runs):
        for cmd, runtime in cmds:
            runtime_stats = run_and_parse_output_from_bench_cmd(cmd, runtime)
            for label, ins_qps, look_qps, rt in runtime_stats:
                key = (label, rt)
                aggregates[key]["runtime"] = rt
                aggregates[key]["inserts"].append(ins_qps)
                aggregates[key]["lookups"].append(look_qps)

    # Build rows
    rows = []
    for (label, rt), vals in aggregates.items():
        ins = vals["inserts"]
        look = vals["lookups"]
        if not ins or not look:
            continue

        ins_avg = mean(ins)
        look_avg = mean(look)
        ins_sd = stdev(ins) if len(ins) >= 2 else 0.0
        look_sd = stdev(look) if len(look) >= 2 else 0.0

        ins_lat = [latency_ns_from_qps(q) for q in ins]
        look_lat = [latency_ns_from_qps(q) for q in look]
        ins_lat_avg = mean(ins_lat)
        look_lat_avg = mean(look_lat)
        ins_lat_sd = stdev(ins_lat) if len(ins_lat) >= 2 else 0.0
        look_lat_sd = stdev(look_lat) if len(look_lat) >= 2 else 0.0

        rows.append({
            "label": label,
            "runtime": rt,
            "ins_qps_avg": ins_avg,
            "ins_qps_sd": ins_sd,
            "ins_lat_avg": ins_lat_avg,
            "ins_lat_sd": ins_lat_sd,
            "look_qps_avg": look_avg,
            "look_qps_sd": look_sd,
            "look_lat_avg": look_lat_avg,
            "look_lat_sd": look_lat_sd,
        })

    # Sort by runtime, then lookup latency
    rows.sort(key=lambda r: (r["runtime"], r["look_lat_avg"]))

    # Print perfectly aligned Markdown table
    header = (
        "| {:<38} | {:<7} | {:<28} | {:<28} | {:<28} | {:<28} |".format(
            "Implementation", "Runtime",
            "Lookups Lat (ns ± sd)", "Lookups QPS (avg ± sd)",
            "Inserts Lat (ns ± sd)", "Inserts QPS (avg ± sd)"
        )
    )
    divider = (
        "|" + "-" * 40 + "|" + "-" * 9 + "|" + "-" * 30 + "|" + "-" * 30 + "|" + "-" * 30 + "|" + "-" * 30 + "|"
    )
    print(header)
    print(divider)

    for r in rows:
        print(
            "| {:<38} | {:<7} | {:<28} | {:<28} | {:<28} | {:<28} |".format(
                r['label'],
                r['runtime'],
                fmt_avg_std(r['look_lat_avg'], r['look_lat_sd'], " ns", 28).strip(),
                fmt_avg_std(r['look_qps_avg'], r['look_qps_sd'], "", 28).strip(),
                fmt_avg_std(r['ins_lat_avg'], r['ins_lat_sd'], " ns", 28).strip(),
                fmt_avg_std(r['ins_qps_avg'], r['ins_qps_sd'], "", 28).strip(),
            )
        )

