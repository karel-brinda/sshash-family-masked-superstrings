# SSHash family with masked superstrings

Supplementary repository: original SSHash-family tools and their masked-superstring (`bigreedy-ms`) modifications, pinned as git submodules.

| Original | Modified |
|---|---|
| [`01_sshash-lite`](https://github.com/jermp/sshash-lite) | [`11_sshash-lite-bigreedy-ms`](https://github.com/karel-brinda/sshash-lite-bigreedy-ms) |
| [`02_sshash`](https://github.com/jermp/sshash) | [`12_sshash-bigreedy-ms`](https://github.com/karel-brinda/sshash-bigreedy-ms) |
| [`03_sshash-rs`](https://github.com/COMBINE-lab/sshash-rs) | [`13_sshash-rs-bigreedy-ms`](https://github.com/karel-brinda/sshash-rs-bigreedy-ms) |

```bash
git clone --recursive https://github.com/karel-brinda/sshash-family-masked-superstrings
```

## Modifications

### 11_sshash-lite-bigreedy-ms

TODO

### 12_sshash-bigreedy-ms

A fork of [SSHash](https://github.com/jermp/sshash) (C++) extended to natively index BiGreedy
masked superstrings, as output by [KmerCamel](https://github.com/ondrejsladky/kmercamel), while
keeping the full dictionary interface (lookup with unique k-mer ids, access, iteration,
streaming and navigational queries).

- **New input format.** `--masked-superstring` indexes one KmerCamel-style masked superstring: a
  single unwrapped FASTA record over `ACGTacgt`, where an uppercase base marks the start of a
  represented k-mer. Only the represented k-mers are indexed; the maximal uppercase runs —
  possibly overlapping intervals of the one superstring — play the role of SSHash's strings.
  Internally, a new `interval_offsets` structure stores interval begins/ends and cumulative
  k-mer counts, and minimizer offsets are encoded as `|string-id|relative-offset|` so a text
  position shared by two overlapping intervals stays unambiguous. A plain SPSS (unitigs,
  simplitigs, eulertigs) is indexed exactly as upstream, as the degenerate case of the same
  representation.

      kmercamel compute -k 31 -o out.msfa input.fa
      ./sshash build -i out.msfa -k 31 -m 16 --masked-superstring --check -o out.sshash
      ./sshash query -i out.sshash -q reads.fastq.gz

- **Caveats.** The mask must be **min-one** (every represented k-mer has exactly one uppercase
  occurrence — the default output of `kmercamel compute`); this is what makes the k-mer ↔ id
  bijection well defined. Non-min-one masks (e.g., `kmercamel maskopt -t max-one`, or matchtig
  input via `kmercamel compute -S`) are not supported and **not detected during construction** —
  `--check` verifies the id bijection and fails on any non-min-one mask. `--weighted` is not
  supported with masked superstrings, and index files are not interchangeable with the base
  SSHash (different internal offsets layout).

### 13_sshash-rs-bigreedy-ms

A hybrid of two codebases: a fork of Rob Patro's
[sshash-rs](https://github.com/COMBINE-lab/sshash-rs) (the Rust implementation of SSHash)
extended with the masked-superstring design of `12_sshash-bigreedy-ms`. It demonstrates that
the design is implementation-independent: both implementations assign identical k-mer ids
(the ranks of the uppercase positions in text order) to the same masked superstring.

- **Same model as `12`, in Rust.** The SPSS stores strings as possibly overlapping intervals of
  one shared text (plain SPSS inputs are the degenerate case), super-kmer offsets are encoded
  as `|string-id|relative-offset|`, and the index format is bumped to 6.0 (not interchangeable
  with upstream sshash-rs).

      kmercamel compute -k 31 -o out.msfa input.fa
      sshash build --input out.msfa -k 31 -m 16 --masked-superstring --output out
      sshash check --index out --input out.msfa --masked-superstring   # verifies the id bijection
      sshash query --index out --query reads.fastq.gz --streaming

- **Benchmark harness.** `scripts/ms-benchmark.sh <spss.fa[.gz]> <k> <m> [query.fa/fq]` compares,
  for one k-mer set, the index built from the given SPSS against the index built from its
  BiGreedy masked superstring (computed with KmerCamel): total size with component breakdown
  (strings / offsets / MPHF / skew index), point-lookup timings, and optionally streaming-query
  timings, plus the min-one id-bijection check.

- **Caveats.** As in `12`: the mask must be **min-one** (KmerCamel's default) and non-min-one
  masks are not detected during construction — run `check --masked-superstring` when the mask
  provenance is uncertain. Weighted dictionaries are not supported with masked superstrings.
