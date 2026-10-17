# AGENTS.md

Supplementary repository for a paper. It pins the SSHash-family tools and their masked-superstring (`bigreedy-ms`) modifications as git submodules. All development and syncing happens **through this repository**, not in separate standalone clones.

## Layout

| Dir | Repo | Role | Branch | Upstream |
|---|---|---|---|---|
| `01_sshash-lite` | jermp/sshash-lite | original | `main` | – |
| `02_sshash` | jermp/sshash | original | `master` | – |
| `03_sshash-rs` | COMBINE-lab/sshash-rs | original | `main` | – |
| `11_sshash-lite-bigreedy-ms` | karel-brinda/sshash-lite-bigreedy-ms | modified | `main` | `01` (jermp/sshash-lite `main`) |
| `12_sshash-bigreedy-ms` | karel-brinda/sshash-bigreedy-ms | modified | `master` | `02` (jermp/sshash `master`) |
| `13_sshash-rs-bigreedy-ms` | karel-brinda/sshash-rs-bigreedy-ms | modified | `main` | `03` (COMBINE-lab/sshash-rs `main`) |

| `00_kmer_sets_benchmark` | jermp/kmer_sets_benchmark | reference benchmark | `main` | – |
| `30_benchmark_membership` | (this repo) | membership benchmark of `0x` vs `1x` | – | – |

Numbering: `0x` = original, `1x` = modification of `0x`. The pinned `0x` commit should be the fork point (merge base) of the corresponding `1x`. `00` is the reference benchmark (only its `src/sshash_benchmark.cpp` and `src/common.hpp` are used; its nested submodules are not needed); `3x` are benchmarks in this repo. See `30_benchmark_membership/README.md`, in particular: the SSHash harness there is a v6-adapted copy of `00`'s (diff it after bumping `00`), and `01` is built from a copy patched with `11`'s PTHash stabilization.

## Rules

- Submodules are pinned to **exact commits**. `.gitmodules` has no `branch =` entries, so do not rely on `git submodule update --remote`.
- Never modify `00`–`03`. They only move when deliberately bumped to a new upstream commit.
- Make code changes only inside `11`–`13`: commit and push in the submodule first, then commit the updated pointer in this repo. Never commit a pointer to a commit that hasn't been pushed.
- `.gitmodules` uses public HTTPS URLs (for readers of the paper). For pushing, set a local SSH push URL in each `1x` (not committed):
  `git -C 1x_... remote set-url --push origin git@github.com:karel-brinda/<repo>.git`
- Each `1x` has a local `upstream` remote pointing to its original repo, plus `git config checkout.defaultRemote origin` (both remotes have the same branch names, so `git switch main` is otherwise ambiguous).
- A submodule checkout is a detached HEAD. Run `git switch <branch>` before committing in it.
- Commit and push only when explicitly asked.

## Common operations

```bash
# fresh clone
git clone --recursive https://github.com/karel-brinda/sshash-family-masked-superstrings
git submodule update --init --recursive

# sync a modification with its upstream (example: 12 ← jermp/sshash)
cd 12_sshash-bigreedy-ms
git switch master
git fetch upstream && git merge upstream/master   # or rebase, per tool
git submodule update --init --recursive
git push
cd .. && git -C 02_sshash fetch && git -C 02_sshash checkout <new fork point>
git add 02_sshash 12_sshash-bigreedy-ms && git commit

# status of everything
git submodule status --recursive
```

## Per-tool notes

### 11_sshash-lite-bigreedy-ms

C++ fork of jermp/sshash-lite (membership-only SSHash; a 2022-era codebase that predates `02`'s
architecture, so nothing here maps 1:1 onto `12`). Unlike `12`/`13` there are **no k-mer ids**:
queries return yes/no only, which is why the min-one requirement is softer here (a build-time
PTHash risk, not silent id corruption). The feature landed via PR
[karel-brinda/sshash-lite-bigreedy-ms#1](https://github.com/karel-brinda/sshash-lite-bigreedy-ms/pull/1)
(commits `e373e52` intervals indexing, `8e53f16` native masked-superstring input, `2260e0a`
masked superstring as default input + `--spss`); post-merge commits added the integration test
(`b737fe4`), README/pixi instructions, and PTHash stabilizations (`59fb520`, `1bbe68f` = current
pin). Reference diff: `git diff 47a594e..main` (fork point = pinned `01`).

Where the changes live:

- `include/builder/bigreedy_ms.hpp` — masked-superstring reader (one unwrapped FASTA record over
  `ACGTacgt`) and maximal-uppercase-run → interval extraction (`[first, last + k)`); rejects
  multi-record/wrapped input and uppercase in the final k−1 positions.
- `include/builder/parse_file.hpp` — `parse_bigreedy_intervals()`: per-interval super-k-mer and
  minimizer emission with offsets absolute in the superstring; `parse_embedding_intervals()`
  validates `--intervals` TSVs (begin-sorted, length ≥ k, overlap < k; nested/duplicate
  intervals rejected). Input dispatch: masked superstring by default, `--spss` for
  one-sequence-per-line, `--intervals <tsv>` (silently wins over `--spss`).
- `include/builder/util.hpp` — `compact_string_pool::build_from_string()`: the superstring is
  stored once; `pieces` holds the interval **ends** only (the regular path's leading 0 was
  removed too, so `pieces` has p entries, not p+1).
- `include/buckets.hpp` — the core invariant:
  `offset_to_contig_end(offset, k) = pieces.next_geq(offset + k)`. Intervals are begin-sorted
  with pairwise overlap < k, so this returns the end of the interval that generated the
  super-k-mer, and the scan window `min(k−m+1, contig_end − offset − k + 1)` confines every
  lookup and streaming extension to fully-in-interval (= represented) k-mers. No mask is stored.
- `include/query/streaming_query_*.hpp` — only the `offset_to_contig_end` signature changed;
  extension logic is bounded by the same window. Debug builds cross-check streaming vs
  non-streaming per k-mer (`assert(is_member == m_res)`).
- `src/check_utils.hpp` — `--check` verifies positives only; junction-negative coverage lives in
  the tests.
- `tests/` — `bigreedy_ms_regression.cpp`: exhaustive 4^5 differential of ordinary vs
  `--intervals` vs masked-superstring indexes (regular and canonical), junction negatives,
  overlapping-interval edge case, input validation; `bigreedy_ms_parser_regression.cpp`:
  run-extraction off-by-ones; `tests/integration/Makefile`: kmercamel → build → query
  end-to-end on `data/ecoli_unitigs_k31.fa.gz` (needs `kmercamel` on `PATH`).

Build & test:

```bash
mkdir build && cd build && cmake .. && make -j && ctest
cd ../tests/integration && make
```

Gotchas and design notes:

- Correctness lemma behind `next_geq(offset + k)`: intervals begin-sorted, each of length ≥ k,
  pairwise overlap < k ⇒ every k-mer occurrence of the superstring lies in at most one interval
  and the smallest end ≥ offset+k belongs to the generating interval. The TSV validator enforces
  exactly these preconditions; mask-derived runs satisfy them structurally (overlap ≤ k−2).
- Base encoding is `(c >> 1) & 3`, i.e. case-insensitive, so the mixed-case superstring is
  stored as-is. `util::is_valid` accepts uppercase only, though: lowercase bases in *query*
  reads count as invalid k-mers (upstream behavior), and `--intervals` mode expects an
  uppercase FASTA.
- Min-one masks make the indexed k-mers duplicate-free. Duplicated represented k-mers (max-one
  masks, or highly repetitive `--spss` input such as tandem repeats) put duplicate keys into the
  skew-index MPHF once a bucket exceeds 2^l super-k-mers, and the PTHash pilot search then
  livelocks (reproduced empirically). There is no build-time guard; matchtigs-level duplication
  is rare enough that buckets stay small in practice.
- PTHash on macOS/ARM is forced single-threaded with a fixed seed for both the minimizers MPHF
  (`59fb520`) and the skew-index MPHFs (`1bbe68f`): the parallel path is unreliable on this
  target.
- `streaming_query.hpp` classifies each read with early exit against `-t`:
  `num_positive_kmers` is not an exhaustive count (at `-t 0`, at most one per read), and the
  negative break at `c_neg >= nk − min` can classify a read with exactly `min` true positive
  k-mers as negative. Upstream sshash-lite behavior; do not use the counters as totals.
- Soft-masked-FASTA foot-gun: the masked superstring being the default input, a single-record
  plain FASTA with lowercase bases is silently interpreted as a mask — use `--spss` for
  ordinary sequences (multi-record or all-uppercase inputs fail loudly instead).
- Validated (2026-10) against KmerCamel v2.3.1 and independent Python ground truth on four
  regimes (dense k=31; 40%-subsampled k=31 and k=15; 30%-subsampled k=11): mask ON set equals
  the k-mer set; 100% of represented k-mers positive (half queried as reverse complements);
  zero false positives over ~490k adversarial junction and random negatives; `--intervals` and
  masked-superstring indexes answer identically; `kmercamel -u` ↔ regular parsing exact;
  `--spss` still rejects k-mers spanning concatenated-sequence boundaries.

### 12_sshash-bigreedy-ms

C++ fork of jermp/sshash with native BiGreedy masked-superstring (MS) indexing. The whole
change is one feature commit (`d34f721`) plus README commits; `git diff <fork point>` is small
(~670 added lines) and is the reference spec for the Rust port in `13`.

Where the changes live:

- `include/masked_superstring.hpp` — parse one unwrapped FASTA record over `ACGTacgt`
  (uppercase = represented k-mer start) and extract the maximal uppercase runs as intervals
  `[begin, end)`: sorted, length ≥ k, consecutive overlaps < k, so each k-mer occurrence of the
  text lies in at most one interval.
- `include/offsets.hpp` — new `struct interval_offsets`: `m_begins` (compact vector),
  `m_ends` and `m_cum` (cumulative k-mer counts; both `endpoints_sequence` with a leading
  zero). Minimizer offsets are encoded as `|string-id|relative-offset|` (like upstream's
  `encoded_offsets`), so a text position anchoring k-mers of two adjacent overlapping intervals
  stays distinguishable. `kmer_id = cum[string_id] + (kmer_offset - string_begin)`.
  `offset_to_id` finds the unique containing interval via successor query on the ends
  (`m_ends.locate(kmer_offset + k - 1)`), then checks `kmer_offset >= string_begin`.
- `include/dictionary_types.hpp` — `dictionary_type` switched to
  `dictionary<default_kmer_t, interval_offsets>`; a plain SPSS is the degenerate case
  `begin_{j+1} == end_j`, so one offsets type serves both inputs (**index files are therefore
  not interchangeable with base SSHash, in either direction**).
- `include/spectrum_preserving_string_set.hpp` — overlap-aware lookup containment (fast path:
  the decoded offset's own string; fallback: `offset_to_id`; a k-mer may lie in *no* string)
  and a string-id-aware k-mer iterator (consecutive intervals can overlap, so the next piece's
  begin may precede the current end).
- `src/builder/encode_strings.cpp` — `encode_strings_masked_superstring`: packs the whole
  superstring once, pushes explicit intervals, computes `num_bits` for the encoded offsets
  (`per_string_id = ceil_log2(num_strings)`, `per_relative_offset = ceil_log2(max_len - m + 1)`,
  sum must be ≤ 64). `src/builder/compute_minimizer_tuples.cpp:~70` is where offsets get
  encoded (`strings_offsets_builder.encode(pos, begin, string_id)`), i.e. *before* sorting and
  dedup — that ordering is load-bearing.
- `tools/build.cpp` — `--masked-superstring` flag; rejects `--weighted`.
- `test/check_from_file.hpp` — `check_correctness_lookup_access_masked_superstring`: k-mer ids
  must be the ranks of uppercase positions in text order (50% of queries as reverse
  complements). **This is the min-one detector**: construction itself does not detect
  non-min-one masks (they silently corrupt ids), so always `--check` on uncertain masks.

Build & test:

```bash
mkdir build && cd build && cmake .. && make -j
./sshash build -i out.msfa -k 31 -m 16 --masked-superstring --check -o out.sshash
```

Gotchas: input must be a single unwrapped FASTA record (no multi-FASTA, no line wrapping);
no uppercase allowed in the final k-1 positions; `--weighted` unsupported with MS; KmerCamel's
default output is min-one but `kmercamel compute -S` (matchtigs) is **not** guaranteed min-one.

### 13_sshash-rs-bigreedy-ms

Hybrid: fork of COMBINE-lab/sshash-rs carrying the masked-superstring design of `12` (use the
C++ diff there as the semantic reference). Unlike C++ upstream, sshash-rs was not generic over
the offsets layout, so the layout was replaced **wholesale** (plain SPSS = degenerate
intervals) rather than dispatched: one code path, serialization format bumped to 6.0
(format-5 indexes must be rebuilt; see `serialization.rs` version history).

Where the changes live (all under `crates/`):

- `sshash-lib/src/masked_superstring.rs` — parser/validator + interval extraction (port of
  `masked_superstring.hpp`).
- `sshash-lib/src/spectrum_preserving_string_set.rs` — the core rework: `begins` (`BitFieldVec`,
  O(1) access), `ends` and `cum` (`EliasFanoOffsets` with a leading zero), `rel_offset_bits`;
  `encode_offset`/`decode_offset` for the `|string-id|relative-offset|` super-kmer offsets;
  `locate_kmer` (successor query on ends + begin check); `first_kmer_id`/`num_kmers` (cum);
  `from_parts` (endpoint-style, degenerate) and `from_interval_vecs` constructors. The old
  `offset_at`/`offsets_len` endpoint view is kept for `tiny-dict` compatibility.
- `sshash-lib/src/builder/minimizer_tuples.rs` — `pos_in_seq` becomes the *encoded* offset at
  tuple-creation time, in both the in-memory and external-sort paths. Encoding before
  sort/dedup is load-bearing: two super-kmers of adjacent overlapping intervals can share an
  absolute minimizer position, and the per-bucket "one tuple per position" dedup must not
  conflate them.
- `sshash-lib/src/sparse_and_skew_index.rs` — the skew-index build decodes tuple offsets back
  to absolute positions before reading k-mers from the text; the `offsets` BitFieldVec now
  stores encoded values (width = `spss.num_bits_per_offset()`).
- `sshash-lib/src/dictionary.rs` — lookup decodes offsets via the SPSS; `try_lookup_at` takes a
  string *hint* (the decoded offset's own string, the fast path) and falls back to
  `locate_kmer`; `kmer_id` is cum-based (`LocatedHit::into_lookup_result`). The streaming
  `BucketCache` stores decoded **absolute** positions. `streaming_query.rs` needed no changes
  (it was already position-based; its debug builds cross-check every streaming answer against a
  point lookup — very useful when touching this code).
- `sshash-lib/src/builder/dictionary_builder.rs` — `build_from_masked_superstring` +
  shared `build_from_spss` pipeline; k-mer counts come from `spss.num_kmers()`, no longer from
  `total_bases - (k-1)*num_strings`.
- `sshash-cli/src/main.rs` — `build --masked-superstring` (reads via needletail, **case
  preserved**; do not reuse `parse_fasta_file`, which uppercases and would erase the mask) and
  `check --masked-superstring` (ids = ranks of uppercase positions; the min-one detector).
- `sshash-lib/tests/masked_superstring_integration.rs` — overlapping intervals, heavy buckets
  (forces the skew path with m=3: only 32 canonical minimizers), content-based membership over
  every text position, streaming-vs-point parity, external-sort path, save/load, input
  validation.

Build & test:

```bash
cargo build --release          # binary: target/release/sshash
cargo test --workspace --exclude sshash-py
scripts/ms-benchmark.sh <spss.fa.gz> 31 15 [query.fa.gz]   # needs kmercamel in PATH
```

Gotchas and facts worth knowing:

- `sshash-py` fails to **link** on macOS (pyo3 dylib, missing Python symbols) — pre-existing
  upstream, identical on a pristine sshash-rs clone; it still compiles against these changes.
  Exclude it from workspace test runs.
- Construction does not detect non-min-one masks; `check --masked-superstring` does.
- Parity with `12` was verified on KmerCamel output for Salmonella enterica (k=31, 4.79M
  k-mers): both implementations pass their id-rank checks on the same file, hence assign
  identical ids; found counts on 4.86M genome queries are identical between the SPSS and MS
  indexes of one k-mer set.
- Measured on the 100-genome Salmonella pangenome (k=31, m=17): MS 8.14 vs simplitigs 8.26
  bits/k-mer total (strings component 2.37 vs 2.69); point lookups at parity (~208 vs ~213 ns
  positive), positive streaming within 8%. The encoded offsets cost ~0.2 bits/k-mer over
  absolute offsets — report component breakdowns, not just totals.
- `m` must be ≤ 31; `ceil_log2(num_strings) + ceil_log2(max_interval_len - m + 1)` must be
  ≤ 64 (asserted at SPSS construction).
