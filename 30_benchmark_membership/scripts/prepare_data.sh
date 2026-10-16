#!/usr/bin/env bash
# Prepare the inputs and queries of one dataset of datasets.tsv:
#   data/<name>/k<k>/unitigs.fa      maximal unitigs (GGCAT)
#   data/<name>/k<k>/simplitigs.fa   greedy simplitigs (GGCAT --simplitigs)
#   data/<name>/k<k>/ms.fa           BiGreedy masked superstring of the unitigs (KmerCamel, min-one)
#   data/<name>/queries/{high,low,mixed}.fastq.gz   streaming queries (mixed: high/low reads 1:1)
# K-mer sets are canonical (both strands), as in every SSHash-family index built by run.sh.
#
# usage: scripts/prepare_data.sh <name>
set -euo pipefail
cd "$(dirname "$0")/.."
name=$1
threads=${THREADS:-$(getconf _NPROCESSORS_ONLN)}
row=$(awk -F'\t' -v n="$name" '$1 == n' datasets.tsv)
[ -n "$row" ] || { echo "dataset '$name' not in datasets.tsv" >&2; exit 1; }
IFS=$'\t' read -r _ k _ genomes high low <<< "$row"
IFS=',' read -r -a genomes <<< "$genomes"

out=data/$name/k$k
mkdir -p "$out" data/$name/queries
run() { pixi run --frozen "$@"; }

# GGCAT 2.2.0 --eulertigs randomly aborts (huge allocation) in about half of the runs; retry
# the non-unitig modes defensively.
# Bad output cannot slip through: count_kmers.py below checks the k-mer counts.
retry() { for i in 1 2 3 4 5 6 7 8; do "$@" && return 0; echo "retry $i: $*" >&2; done; return 1; }
ggcat() { run ggcat build -k "$k" -s 1 -j "$threads" -t "$out/ggcat_tmp" "$@" "${genomes[@]}"; }
ggcat -o "$out/unitigs.fa"
retry ggcat --simplitigs -o "$out/simplitigs.fa"
rm -rf "$out/ggcat_tmp"
rm -f "$out"/*.stats.log
run kmercamel compute -k "$k" -o "$out/ms.fa" "$out/unitigs.fa"

# Sanity: all three inputs must represent the same k-mer count.
python3 -I scripts/count_kmers.py "$k" "$out/unitigs.fa" "$out/simplitigs.fa" "$out/ms.fa" | tee "$out/num_kmers.tsv"

cp "$high" data/$name/queries/high.fastq.gz
cp "$low" data/$name/queries/low.fastq.gz
python3 -I scripts/interleave_fastq.py data/$name/queries/high.fastq.gz data/$name/queries/low.fastq.gz \
    data/$name/queries/mixed.fastq.gz
