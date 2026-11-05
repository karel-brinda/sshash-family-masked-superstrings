salmonella_enterica, k=31, 4,787,534 k-mers; medians of 3 runs; Δ = change vs the 0x original

| tool | input | bits/kmer | build (s) | build RSS (MB) | pos. lookup (ns) | neg. lookup (ns) | access (ns) | stream high (ns/kmer) | stream low (ns/kmer) | stream mixed (ns/kmer) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01 sshash-lite | unitigs | 5.97 | 0.70 | 60.06 | 309 | 312 | – | 98.92 | 117 | 101 |
| 11 sshash-lite-bigreedy-ms | unitigs | 5.97 | 0.80 | 59.45 | 309 | 314 | – | 97.18 | 121 | 101 |
| Δ vs 01 | vs unitigs | +0.0% | +14.3% | -1.0% | -0.2% | +0.6% | – | -1.8% | +4.1% | -0.5% |
| 01 sshash-lite | simplitigs | 5.95 | 0.71 | 57.84 | 308 | 315 | – | 97.48 | 117 | 100 |
| 11 sshash-lite-bigreedy-ms | simplitigs | 5.95 | 0.80 | 57.78 | 311 | 312 | – | 97.81 | 117 | 101 |
| Δ vs 01 | vs simplitigs | +0.0% | +12.7% | -0.1% | +1.0% | -1.0% | – | +0.3% | -0.3% | +0.5% |
| 11 sshash-lite-bigreedy-ms | ms | 5.94 | 0.75 | 68.41 | 300 | 303 | – | 96.41 | 116 | 98.89 |
| Δ vs 01 | vs simplitigs | -0.1% | +5.6% | +18.3% | -2.5% | -4.0% | – | -1.1% | -1.3% | -1.4% |
| 02 sshash | unitigs | 5.12 | 0.30 | 58.75 | 201 | 120 | 427 | 26.90 | 41.13 | 29.72 |
| 12 sshash-bigreedy-ms | unitigs | 5.87 | 0.33 | 58.39 | 314 | 204 | 59.08 | 29.04 | 51.58 | 32.44 |
| Δ vs 02 | vs unitigs | +14.8% | +10.0% | -0.6% | +56.2% | +69.9% | -86.2% | +7.9% | +25.4% | +9.2% |
| 02 sshash | simplitigs | 5.09 | 0.30 | 58.22 | 204 | 116 | 361 | 27.19 | 39.41 | 28.57 |
| 12 sshash-bigreedy-ms | simplitigs | 5.72 | 0.30 | 58.75 | 314 | 207 | 60.69 | 29.46 | 50.26 | 32.37 |
| Δ vs 02 | vs simplitigs | +12.4% | +0.0% | +0.9% | +53.7% | +77.7% | -83.2% | +8.4% | +27.5% | +13.3% |
| 12 sshash-bigreedy-ms | ms | 5.83 | 0.31 | 67.86 | 283 | 192 | 62.34 | 29.28 | 47.77 | 31.75 |
| Δ vs 02 | vs simplitigs | +14.6% | +3.3% | +16.6% | +38.5% | +65.1% | -82.7% | +7.7% | +21.2% | +11.1% |
| 03 sshash-rs | unitigs | 5.00 | 0.50 | 123 | 296 | 150 | – | 24.87 | 42.26 | 27.62 |
| 13 sshash-rs-bigreedy-ms | unitigs | 5.69 | 0.49 | 123 | 264 | 214 | – | 26.92 | 50.58 | 29.90 |
| Δ vs 03 | vs unitigs | +13.7% | -2.0% | +0.0% | -10.8% | +42.5% | – | +8.2% | +19.7% | +8.3% |
| 03 sshash-rs | simplitigs | 4.99 | 0.48 | 122 | 302 | 148 | – | 24.89 | 40.52 | 27.32 |
| 13 sshash-rs-bigreedy-ms | simplitigs | 5.56 | 0.46 | 125 | 254 | 211 | – | 26.96 | 47.38 | 29.77 |
| Δ vs 03 | vs simplitigs | +11.3% | -4.2% | +2.6% | -16.0% | +42.9% | – | +8.3% | +16.9% | +9.0% |
| 13 sshash-rs-bigreedy-ms | ms | 5.65 | 0.47 | 126 | 247 | 201 | – | 26.08 | 46.75 | 29.44 |
| Δ vs 03 | vs simplitigs | +13.1% | -2.1% | +3.4% | -18.3% | +36.0% | – | +4.8% | +15.4% | +7.7% |

All indexes report identical positive k-mer counts on every streaming workload: high 2,032,354, low 0, mixed 2,032,354.
