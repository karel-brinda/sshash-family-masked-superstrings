#!/usr/bin/env bash
# Build every (tool, input) index of one dataset and benchmark it.
#
#   tools:  01 sshash v3  02 sshash-lite  03 sshash v4  04 sshash v6  05 sshash-rs  (originals,
#           in chronological order of the SSHash versions)
#           11, 12, 13, 14, 15  their masked-superstring modifications
#   inputs: unitigs, simplitigs (all tools), ms (1x only)
#
# Outputs, in results/<name>/k<k>/:
#   builds.tsv     build time / peak RSS / index size per (tool, input)
#   raw.jsonl      one JSON line per benchmark run (lookups, streaming high/low/mixed) and repetition
#   queries.txt    the positive lookup queries shared by all tools
#
# usage: scripts/run.sh <name>      (env: REPEATS=3, THREADS=<all cores>,
#                                    TOOLS="01 02 03 04 05 11 12 13 14 15",
#                                    BENCH_NUM_QUERIES=<num_queries of datasets.tsv>,
#                                    REUSE_INDEXES=1 to keep the indexes and builds.tsv of a previous run)
set -euo pipefail
cd "$(dirname "$0")/.."
here=$PWD
name=$1
repeats=${REPEATS:-3}
threads=${THREADS:-$(getconf _NPROCESSORS_ONLN)}
tools=${TOOLS:-"01 02 03 04 05 11 12 13 14 15"}
# SSHash v4 (03, 13) requires a power of 2 threads: the largest one <= THREADS.
threads_p2=1; while [ $((threads_p2 * 2)) -le "$threads" ]; do threads_p2=$((threads_p2 * 2)); done
row=$(awk -F'\t' -v n="$name" '$1 == n' datasets.tsv)
[ -n "$row" ] || { echo "dataset '$name' not in datasets.tsv" >&2; exit 1; }
IFS=$'\t' read -r _ k m _ _ _ num_queries <<< "$row"
# Random lookup / access queries per benchmark (reference: 1M), read by every harness.
export BENCH_NUM_QUERIES=${BENCH_NUM_QUERIES:-$num_queries}

data=$here/data/$name/k$k
queries=$here/data/$name/queries
out=$here/results/$name/k$k
idx=$out/indexes
mkdir -p "$idx" "$out/logs" "$out/tmp"
bin=$here/build
timecmd=$(command -v gtime || echo /usr/bin/time)

inputs_of() { case $1 in 0*) echo "unitigs simplitigs" ;; 1*) echo "unitigs simplitigs ms" ;; esac; }

# Index file name (C++) or prefix (Rust: <prefix>.ssi + <prefix>.ssi.mphf).
index_of() { echo "$idx/$1.$2.idx"; }

build_index() {
    local tool=$1 input=$2 in=$data/$2.fa index; index=$(index_of "$1" "$2")
    case $tool in
        02) cmd=("$bin/sshash-lite-02" build "$in" "$k" "$m" --canonical-parsing -d "$out/tmp" -o "$index") ;;
        12) cmd=("$bin/sshash-lite-12" build "$in" "$k" "$m" --canonical-parsing -d "$out/tmp" -o "$index")
            [ "$input" = ms ] || cmd+=(--spss) ;;
        04 | 14)
            cmd=("$bin/sshash-$tool" build -i "$in" -k "$k" -m "$m" -t "$threads" -d "$out/tmp" -o "$index")
            [ "$input" = ms ] && cmd+=(--masked-superstring) ;;
        01 | 11)  # SSHash v3: positional arguments, single-threaded (as SSHash-Lite)
            cmd=("$bin/sshash-$tool-build" "$in" "$k" "$m" --canonical-parsing -d "$out/tmp" -o "$index")
            [ "$input" = ms ] && cmd+=(--masked-superstring) ;;
        03 | 13)
            cmd=("$bin/sshash-$tool" build -i "$in" -k "$k" -m "$m" --canonical -t "$threads_p2" -d "$out/tmp" -o "$index")
            [ "$input" = ms ] && cmd+=(--masked-superstring) ;;
        05 | 15)
            cmd=("$bin/sshash-rs-$tool" build -i "$in" -k "$k" -m "$m" -t "$threads" -o "$index")
            [ "$input" = ms ] && cmd+=(--masked-superstring) ;;
    esac
    local log=$out/logs/build.$tool.$input
    "$timecmd" -v -o "$log.time" "${cmd[@]}" > "$log.log" 2>&1
    local bytes; bytes=$(python3 -I -c 'import glob, os, sys; print(sum(map(os.path.getsize, glob.glob(sys.argv[1] + "*"))))' "$index")
    local secs rss
    secs=$(awk -F': ' '/Elapsed \(wall clock\)/ {n = split($2, a, ":"); s = 0; for (i = 1; i <= n; i++) s = s * 60 + a[i]; print s}' "$log.time")
    rss=$(awk -F': ' '/Maximum resident set size/ {print $2}' "$log.time")  # KB
    printf '%s\t%s\t%s\t%s\t%s\n' "$tool" "$input" "$secs" "$rss" "$bytes" >> "$out/builds.tsv"
}

# Run one benchmark in a scratch working directory (03/13/04/14 write queries.txt there) and
# append its JSON line, tagged with tool/input/workload/repetition, to raw.jsonl.
bench() {
    local tool=$1 input=$2 workload=$3 rep=$4; shift 4
    local cwd=$out/tmp/bench; mkdir -p "$cwd"
    local log=$out/logs/bench.$tool.$input.$workload.$rep
    (cd "$cwd" && "$bin/bench-$tool" -i "$(index_of "$tool" "$input")" "$@" > "$log.out" 2> "$log.err")
    grep '^{' "$log.err" | tail -1 | python3 -I -c '
import json, sys
d = json.loads(sys.stdin.read())
d.update(tool=sys.argv[1], input=sys.argv[2], workload=sys.argv[3], rep=int(sys.argv[4]))
print(json.dumps(d))' "$tool" "$input" "$workload" "$rep" >> "$out/raw.jsonl"
}

: > "$out/raw.jsonl"

if [ "${REUSE_INDEXES:-0}" = 1 ] && [ -s "$out/builds.tsv" ]; then
    echo "reusing the indexes of $out" >&2
else
    printf 'tool\tinput\tbuild_s\tbuild_rss_kb\tindex_bytes\n' > "$out/builds.tsv"
    for tool in $tools; do
        for input in $(inputs_of "$tool"); do
            echo "[$(date +%T)] build $tool $input" >&2
            build_index "$tool" "$input"
        done
    done
fi
cp "$data/num_kmers.tsv" "$out/"

# Shared positive queries: 1M k-mers sampled by the reference harness (04 on unitigs), half of
# them reverse-complemented. The tools with k-mer ids (01/11, 03/13, 04/14) sample their own from
# the same distribution.
(cd "$out/tmp" && "$bin/bench-04" -i "$(index_of 04 unitigs)" > /dev/null 2>&1 && mv queries.txt "$out/")

for rep in $(seq 1 "$repeats"); do
    for tool in $tools; do
        for input in $(inputs_of "$tool"); do
            echo "[$(date +%T)] rep $rep bench $tool $input" >&2
            case $tool in
                01 | 11 | 03 | 13 | 04 | 14) bench "$tool" "$input" lookup "$rep" ;;  # samples its own positives
                *) bench "$tool" "$input" lookup "$rep" -p "$out/queries.txt" ;;
            esac
            for w in high low mixed; do
                bench "$tool" "$input" "$w" "$rep" -q "$queries/$w.fastq.gz"
            done
        done
    done
done
rm -rf "$out/tmp"

python3 -I scripts/make_table.py "$out" | tee "$out/table.md"
