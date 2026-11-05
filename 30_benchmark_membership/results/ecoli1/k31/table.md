ecoli1, k=31, 4,877,400 k-mers; medians of 3 runs; Δ = change vs the 0x original

| tool | input | bits/kmer | build (s) | build RSS (MB) | pos. lookup (ns) | neg. lookup (ns) | access (ns) | stream high (ns/kmer) | stream low (ns/kmer) | stream mixed (ns/kmer) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01 sshash-lite | unitigs | 5.96 | 0.72 | 60.47 | 307 | 316 | – | 109 | 118 | 111 |
| 11 sshash-lite-bigreedy-ms | unitigs | 5.96 | 0.71 | 61.86 | 310 | 313 | – | 111 | 117 | 113 |
| Δ vs 01 | vs unitigs | +0.0% | -1.4% | +2.3% | +1.1% | -0.8% | – | +2.5% | -0.7% | +1.1% |
| 01 sshash-lite | simplitigs | 5.94 | 0.72 | 60.38 | 307 | 309 | – | 110 | 117 | 112 |
| 11 sshash-lite-bigreedy-ms | simplitigs | 5.94 | 0.72 | 60.31 | 297 | 300 | – | 108 | 115 | 110 |
| Δ vs 01 | vs simplitigs | -0.0% | +0.0% | -0.1% | -3.5% | -2.7% | – | -1.4% | -1.3% | -1.7% |
| 11 sshash-lite-bigreedy-ms | ms | 5.93 | 0.73 | 70.11 | 296 | 307 | – | 109 | 115 | 110 |
| Δ vs 01 | vs simplitigs | -0.0% | +1.4% | +16.1% | -3.8% | -0.5% | – | -1.2% | -1.5% | -2.0% |
| 02 sshash | unitigs | 5.10 | 0.32 | 59.05 | 203 | 117 | 406 | 33.10 | 42.15 | 33.94 |
| 12 sshash-bigreedy-ms | unitigs | 5.74 | 0.30 | 59.31 | 306 | 203 | 59.10 | 36.39 | 47.66 | 40.62 |
| Δ vs 02 | vs unitigs | +12.5% | -6.3% | +0.4% | +50.9% | +74.5% | -85.5% | +9.9% | +13.1% | +19.7% |
| 02 sshash | simplitigs | 5.08 | 0.28 | 64.44 | 201 | 123 | 356 | 31.53 | 40.58 | 34.01 |
| 12 sshash-bigreedy-ms | simplitigs | 5.60 | 0.29 | 65.80 | 313 | 207 | 59.15 | 38.39 | 52.68 | 40.41 |
| Δ vs 02 | vs simplitigs | +10.2% | +3.6% | +2.1% | +55.6% | +68.5% | -83.4% | +21.7% | +29.8% | +18.8% |
| 12 sshash-bigreedy-ms | ms | 5.71 | 0.31 | 66.84 | 280 | 195 | 61.10 | 36.07 | 48.02 | 39.51 |
| Δ vs 02 | vs simplitigs | +12.3% | +10.7% | +3.7% | +39.1% | +58.5% | -82.8% | +14.4% | +18.3% | +16.2% |
| 03 sshash-rs | unitigs | 5.05 | 0.51 | 123 | 301 | 155 | – | 31.10 | 40.56 | 33.09 |
| 13 sshash-rs-bigreedy-ms | unitigs | 5.62 | 0.48 | 123 | 262 | 210 | – | 34.65 | 48.75 | 39.88 |
| Δ vs 03 | vs unitigs | +11.3% | -5.9% | +0.5% | -12.8% | +35.4% | – | +11.4% | +20.2% | +20.5% |
| 03 sshash-rs | simplitigs | 5.07 | 0.50 | 123 | 295 | 149 | – | 29.28 | 39.66 | 33.10 |
| 13 sshash-rs-bigreedy-ms | simplitigs | 5.52 | 0.46 | 122 | 238 | 210 | – | 32.71 | 49.29 | 38.11 |
| Δ vs 03 | vs simplitigs | +8.9% | -8.0% | -0.8% | -19.5% | +40.6% | – | +11.7% | +24.3% | +15.1% |
| 13 sshash-rs-bigreedy-ms | ms | 5.63 | 0.47 | 126 | 239 | 205 | – | 32.66 | 46.60 | 36.23 |
| Δ vs 03 | vs simplitigs | +11.0% | -6.0% | +2.3% | -19.1% | +37.2% | – | +11.5% | +17.5% | +9.5% |

All indexes report identical positive k-mer counts on every streaming workload: high 662,056, low 0, mixed 662,056.
