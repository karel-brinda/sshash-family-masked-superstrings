# 30_benchmark_membership

Membership benchmark of the SSHash family: each original (`0x`) vs its masked-superstring
modification (`1x`), on the same k-mer sets. The methodology and code follow
[`00_kmer_sets_benchmark`](https://github.com/jermp/kmer_sets_benchmark) (pinned as a submodule):
each library is compiled into a small harness that times queries in-process.

Two questions:

1. **Regression on ordinary input:** `1x` vs `0x` on the same unitigs / simplitigs.
2. **Masked superstrings:** `1x` on the BiGreedy masked superstring vs `0x` on simplitigs.

## Quick start

```bash
pixi install                      # GGCAT, KmerCamel (data preparation only)
scripts/build.sh                  # all tools and harnesses -> build/
scripts/prepare_data.sh ecoli1    # inputs and queries of a dataset of datasets.tsv -> data/
scripts/run.sh ecoli1             # indexes, benchmarks, table -> results/ecoli1/k31/
```

`run.sh` takes `REPEATS` (default 3), `THREADS` (build threads, default all cores) and `TOOLS`
(default `01 02 03 11 12 13`). Requires CMake, a C++20 compiler, zlib, Rust ≥ 1.88 and GNU time
(`gtime` on macOS: `brew install gnu-time`).

## Matrix

| Input | `01` | `11` | `02` | `12` | `03` | `13` |
|---|---|---|---|---|---|---|
| unitigs (GGCAT) | ✓ | ✓ `--spss` | ✓ | ✓ | ✓ | ✓ |
| simplitigs (GGCAT `--simplitigs`) | ✓ | ✓ `--spss` | ✓ | ✓ | ✓ | ✓ |
| BiGreedy masked superstring (KmerCamel, min-one) | – | ✓ | – | ✓ `--masked-superstring` | – | ✓ `--masked-superstring` |

All indexes are built with the same `k`, `m` (per dataset, in `datasets.tsv`) and both strands
(`--canonical-parsing` for SSHash-Lite; SSHash v6 and sshash-rs always match both strands).

Metrics, following the reference: index size on disk (bits/k-mer), build time and peak RSS,
random positive lookup (N k-mers, half reverse-complemented), random negative lookup (N random
k-mers), random access (N k-mer ids; SSHash only), and streaming queries over FASTQ reads with
high-, low- and mixed-hit workloads (mixed = high- and low-hit reads interleaved 1:1). Lookup
timings are averages over 5 passes; every benchmark runs `REPEATS` times and the table reports
medians.

N is set per dataset (`num_queries` in `datasets.tsv`), passed to all harnesses as
`$BENCH_NUM_QUERIES`; set it in the environment of `run.sh` to override (the reference uses 1M).
The values are chosen so that a run takes about a minute per pass on a laptop. For example,
`ecoli1` at 100K with 3 repeats takes 45 s, vs about 5 min at 1M, and the medians agree within
a few percent. The pangenome uses 20K because its lookups are about 5× slower. Index builds
are separate: `REUSE_INDEXES=1 scripts/run.sh <name>` reruns the benchmarks on the indexes of a
previous run.

## Harnesses

| Tools | Harness | Relation to the reference |
|---|---|---|
| `02`, `12` | [`src/sshash_benchmark.cpp`](src/sshash_benchmark.cpp) | copy of the reference, adapted to the SSHash v6 API (marked `v6:`) |
| `01`, `11` | [`src/sshash_lite_benchmark.cpp`](src/sshash_lite_benchmark.cpp) | port: membership only, no access |
| `03`, `13` | [`rust/src/main.rs`](rust/src/main.rs) | port: no access (sshash-rs has no id → k-mer access) |

The reference harness on `main` does not compile against current SSHash (v6 removed
`dictionary::canonical()` and the canonical/regular `streaming_query` variants, and renamed
`load_dictionary`), hence the copy. After bumping `00_kmer_sets_benchmark`, review upstream changes
with:

```bash
diff -u ../00_kmer_sets_benchmark/src/sshash_benchmark.cpp src/sshash_benchmark.cpp
```

Only the SSHash harness of `00_kmer_sets_benchmark` is used; its nested submodules (SBWT, FMSI,
its own SSHash) are not needed and need not be initialized.

**Shared queries.** The `02` harness samples positive queries through `access` and writes them to
`queries.txt`. `run.sh` takes this file from `02` on unitigs and passes it (`-p`) to the
SSHash-Lite and Rust harnesses, which cannot sample by id. `02`/`12` resample from the same
distribution on every run. Streaming workloads are identical for all tools. As a correctness
check, `make_table.py` verifies that every index reports the same number of positive k-mers on
every streaming workload.

## Fairness notes

- **Same flags.** All C++ tools, CLIs included, are compiled by [`CMakeLists.txt`](CMakeLists.txt)
  from their submodule sources with identical flags (`-O3`, `-march=native` / `-mcpu=native`,
  C++20 for SSHash-Lite, C++17 for SSHash). Their own build systems are not used. Rust tools and
  harnesses are built with `-C target-cpu=native` and the `release` profile of sshash-rs.
- **`01` is built with the PTHash stabilization of `11`.** `11` makes PTHash single-threaded with a
  fixed seed (commits `59fb520`, `1bbe68f`), because the parallel path is unreliable on macOS/ARM.
  Unpatched `01` peaks at >4.5 GB and gets killed even on E. coli. CMake builds `01` from a copy
  patched with [`patches/01-pthash-stabilization.patch`](patches/01-pthash-stabilization.patch)
  (the submodule is untouched), so `01` and `11` differ only in the masked-superstring feature.
  To build pristine `01`, pass `-DBENCH_STABILIZE_01=OFF`.
- **Build time.** Build times are reported, but construction is not the focus. SSHash-Lite PTHash
  is single-threaded (see above), while `02`/`12`/`03`/`13` use `THREADS`.
- **Platform.** Report numbers from Linux x86-64 and use macOS for development. Small datasets
  largely fit in cache: use them to check the pipeline, not to draw conclusions.

## Layout

```
datasets.tsv        name, k, m, genomes (comma-separated), high-/low-hit read sets, num_queries
scripts/            build.sh, prepare_data.sh, run.sh, make_table.py, helpers
src/, rust/         harnesses
patches/            build-time patch of 01 (see above)
data/               generated inputs and queries (ignored)
results/<name>/k<k>/  builds.tsv, raw.jsonl, table.md (committed); indexes, logs (ignored)
```

## Data

- **Inputs.** `prepare_data.sh` computes, from the genomes of a dataset: maximal unitigs
  (`ggcat build -s 1`), greedy simplitigs (`ggcat build -s 1 --simplitigs`), and the BiGreedy
  masked superstring of the unitigs (`kmercamel compute`, min-one mask). It checks that all three
  represent the same number of k-mers.
- **Development datasets.** `ecoli1` and `salmonella_enterica` (single genomes, about 4.8M
  31-mers, from `02_sshash/data`). Their read sets are the 10K-read FASTQs in
  `01_sshash-lite/data/queries`:

  | Dataset | High-hit set | Low-hit set |
  |---|---|---|
  | ecoli1 | SRR9873306 (55% of k-mers found) | SRR5833294, human (0%) |
  | salmonella_enterica | SRR14373875 (80%) | SRR5833294, human (0%) |

  These are too small for reliable streaming timings.
- **`ecoli_pangenome_661k_r0.1`.** A 10% k-mer subsample of the 661k E. coli pangenome
  ([Zenodo 14722244](https://zenodo.org/records/14722244),
  `ecoli_pangenome_661k_HQ.unitigs_k128_subsampled_k31_r0.1.fa.xz`, 534 MB). Download it into
  `data/downloads/` before running `prepare_data.sh`:

  ```bash
  curl -L -o data/downloads/ecoli_pangenome_661k_HQ.unitigs_k128_subsampled_k31_r0.1.fa.xz \
      'https://zenodo.org/records/14722244/files/ecoli_pangenome_661k_HQ.unitigs_k128_subsampled_k31_r0.1.fa.xz?download=1'
  ```

  The dataset has 55.1M 31-mers in 49.5M unitigs or simplitigs (1.54 Gbp), and a 475 Mbp
  masked superstring. Subsampling leaves an extremely fragmented set, the opposite extreme of
  the single genomes. Read sets are those of `ecoli1`: 9% of k-mers are found in the high-hit
  set and 0.3% in the low-hit set. Preparation takes about 13 min on a laptop, mostly
  KmerCamel, which is single-threaded and peaks at 2.2 GB. The 15 builds take about 8.5 min.
- Larger datasets and other values of k are added as rows of `datasets.tsv`. Genomes may be
  `.xz`: GGCAT cannot read xz and picks the format from the file extension, so
  `prepare_data.sh` streams xz files through named FIFOs ending in `.fa`.
- **GGCAT caveats.** `-s 1` is required: the default minimum multiplicity of 2 silently drops
  k-mers that occur once. GGCAT 2.2.0 non-unitig modes randomly abort with a huge allocation, so
  `prepare_data.sh` retries them. On the decompressed 2.3 GB pangenome file, GGCAT once hung (0%
  CPU at the end of reads bucketing); the run through a FIFO did not.
- **Memory.** Random lookups on indexes of about 1 GB are sensitive to memory pressure. On a
  16 GB laptop with swap in use, pangenome lookup times varied by up to 90% between repeats,
  while streaming, size and build numbers stayed stable. Benchmark on an otherwise idle machine
  with enough RAM.
