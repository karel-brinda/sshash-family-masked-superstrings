#!/usr/bin/env bash
# Build all tools of the SSHash family and their benchmark harnesses into build/.
# C++ (01, 02, 11, 12) via CMake with shared flags; Rust (03, 13) via cargo with -C target-cpu=native.
set -euo pipefail
cd "$(dirname "$0")/.."
root=..
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN)}

# Number of random lookup / access queries of every harness (reference: 1000000).
export BENCH_NUM_QUERIES=${BENCH_NUM_QUERIES:-100000}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBENCH_NUM_QUERIES=$BENCH_NUM_QUERIES
cmake --build build -j "$jobs"

export RUSTFLAGS="${RUSTFLAGS:-} -C target-cpu=native"
for id in 03 13; do
    case $id in
        03) dir=03_sshash-rs ;;
        13) dir=13_sshash-rs-bigreedy-ms ;;
    esac
    cargo build --release --manifest-path rust/bench-$id/Cargo.toml --target-dir build/rust-$id
    cargo build --release --manifest-path $root/$dir/Cargo.toml -p sshash --target-dir build/rust-$id
    cp build/rust-$id/release/bench-$id build/bench-$id
    cp build/rust-$id/release/sshash build/sshash-rs-$id
done

ls -1 build | grep -E '^(bench|sshash)-'
