"""Summarize results/<name>/k<k>/ (builds.tsv, raw.jsonl, num_kmers.tsv) as a Markdown table.

Timings are medians over repetitions. Each 1x row has a Δ row against its 0x original on the
same input; the 1x masked-superstring row is compared with the 0x original on simplitigs.
Also checks that every index answers every workload with the same number of positive k-mers.

usage: python3 make_table.py results/<name>/k<k>
"""

import csv
import json
import statistics
import sys
from collections import defaultdict
from pathlib import Path

out = Path(sys.argv[1])
NAMES = {"01": "sshash-lite", "02": "sshash", "03": "sshash-rs",
         "11": "sshash-lite-bigreedy-ms", "12": "sshash-bigreedy-ms", "13": "sshash-rs-bigreedy-ms"}
WORKLOADS = ["high", "low", "mixed"]

with open(out / "num_kmers.tsv") as f:
    num_kmers = {int(r["num_kmers"]) for r in csv.DictReader(f, delimiter="\t")}
assert len(num_kmers) == 1, num_kmers
num_kmers = num_kmers.pop()

with open(out / "builds.tsv") as f:
    builds = {(r["tool"], r["input"]): r for r in csv.DictReader(f, delimiter="\t")}

samples = defaultdict(list)  # (tool, input, metric) -> values
positives = defaultdict(set)  # (workload) -> {(tool, input, num_positive_kmers)}
with open(out / "raw.jsonl") as f:
    for line in f:
        d = json.loads(line)
        key = (d["tool"], d["input"])
        if d["workload"] == "lookup":
            samples[key + ("pos",)].append(float(d["positive lookup (avg_nanosec_per_kmer)"]))
            samples[key + ("neg",)].append(float(d["negative lookup (avg_nanosec_per_kmer)"]))
            if "access (avg_nanosec_per_kmer)" in d:
                samples[key + ("access",)].append(float(d["access (avg_nanosec_per_kmer)"]))
        else:
            samples[key + (d["workload"],)].append(float(d["avg_nanosec_per_kmer"]))
            positives[d["workload"]].add(key + (int(d["num_positive_kmers"]),))


def metrics(tool, inp):
    b = builds[(tool, inp)]
    row = {
        "bits/kmer": int(b["index_bytes"]) * 8 / num_kmers,
        "build s": float(b["build_s"]),
        "build MB": int(b["build_rss_kb"]) / 1024,
    }
    for m in ["pos", "neg", "access"] + WORKLOADS:
        v = samples.get((tool, inp, m))
        row[m] = statistics.median(v) if v else None
    return row


COLS = ["bits/kmer", "build s", "build MB", "pos", "neg", "access"] + WORKLOADS
HEADER = ["tool", "input", "bits/kmer", "build (s)", "build RSS (MB)", "pos. lookup (ns)",
          "neg. lookup (ns)", "access (ns)"] + [f"stream {w} (ns/kmer)" for w in WORKLOADS]


def fmt(v):
    return "–" if v is None else f"{v:.2f}" if v < 100 else f"{v:.0f}"


def delta(a, b):
    return "–" if a is None or b is None else f"{100 * (a - b) / b:+.1f}%"


print(f"{out.parent.name}, k={out.name[1:]}, {num_kmers:,} k-mers; medians of "
      f"{len(samples[next(iter(samples))])} runs; Δ = change vs the 0x original\n")
print("| " + " | ".join(HEADER) + " |")
print("|" + "---|" * len(HEADER))
for orig in ["01", "02", "03"]:
    mod = str(int(orig) + 10).zfill(2)
    for tool, inp in [(orig, "unitigs"), (mod, "unitigs"), (orig, "simplitigs"), (mod, "simplitigs"),
                      (mod, "ms")]:
        if (tool, inp) not in builds:
            continue
        r = metrics(tool, inp)
        print(f"| {tool} {NAMES[tool]} | {inp} | " + " | ".join(fmt(r[c]) for c in COLS) + " |")
        if tool == mod:
            ref_inp = "simplitigs" if inp == "ms" else inp
            if (orig, ref_inp) in builds:
                ref = metrics(orig, ref_inp)
                print(f"| Δ vs {orig} | vs {ref_inp} | " + " | ".join(delta(r[c], ref[c]) for c in COLS) + " |")

ok = True
for w, s in sorted(positives.items()):
    counts = {c for *_, c in s}
    if len(counts) != 1:
        ok = False
        print(f"\n**MISMATCH** in positive k-mers for workload '{w}': {sorted(s)}")
if ok:
    print("\nAll indexes report identical positive k-mer counts on every streaming workload: "
          + ", ".join(f"{w} {next(iter(s))[-1]:,}" for w, s in sorted(positives.items())) + ".")
