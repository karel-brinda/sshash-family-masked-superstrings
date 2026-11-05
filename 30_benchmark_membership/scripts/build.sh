#!/usr/bin/env bash
# Build all tools of the SSHash family and their benchmark harnesses into build/.
# C++ (01-04, 11-14) via CMake with shared flags; Rust (05, 15) via cargo with -C target-cpu=native.
set -euo pipefail
cd "$(dirname "$0")/.."
root=..
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN)}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$jobs"

export RUSTFLAGS="${RUSTFLAGS:-} -C target-cpu=native"
for id in 05 15; do
    case $id in
        05) dir=05_sshash-rs ;;
        15) dir=15_sshash-rs-bigreedy-ms ;;
    esac
    cargo build --release --manifest-path rust/bench-$id/Cargo.toml --target-dir build/rust-$id
    cargo build --release --manifest-path $root/$dir/Cargo.toml -p sshash --target-dir build/rust-$id
    cp build/rust-$id/release/bench-$id build/bench-$id
    cp build/rust-$id/release/sshash build/sshash-rs-$id
done

ls -1 build | grep -E '^(bench|sshash)-'
