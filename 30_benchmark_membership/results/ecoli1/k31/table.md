ecoli1, k=31, 4,877,400 k-mers; medians of 3 runs; Δ = change vs the 0x original

| tool | input | bits/kmer | build (s) | build RSS (MB) | pos. lookup (ns) | neg. lookup (ns) | access (ns) | stream high (ns/kmer) | stream low (ns/kmer) | stream mixed (ns/kmer) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01 sshash-lite | unitigs | 5.96 | 1.30 | 60.94 | 312 | 309 | – | 109 | 119 | 113 |
| 11 sshash-lite-bigreedy-ms | unitigs | 5.96 | 1.02 | 59.92 | 337 | 318 | – | 111 | 118 | 111 |
| Δ vs 01 | vs unitigs | +0.0% | -21.5% | -1.7% | +7.9% | +2.6% | – | +2.1% | -0.2% | -1.3% |
| 01 sshash-lite | simplitigs | 5.94 | 0.71 | 60.75 | 312 | 309 | – | 109 | 113 | 112 |
| 11 sshash-lite-bigreedy-ms | simplitigs | 5.94 | 0.71 | 60.41 | 303 | 305 | – | 108 | 115 | 109 |
| Δ vs 01 | vs simplitigs | -0.0% | +0.0% | -0.6% | -2.9% | -1.5% | – | -1.0% | +2.0% | -3.0% |
| 11 sshash-lite-bigreedy-ms | ms | 5.93 | 0.71 | 71.88 | 306 | 304 | – | 107 | 117 | 112 |
| Δ vs 01 | vs simplitigs | -0.0% | +0.0% | +18.3% | -1.8% | -1.7% | – | -2.0% | +3.1% | -0.3% |
| 02 sshash | unitigs | 5.10 | 0.61 | 59.25 | 201 | 116 | 404 | 32.16 | 43.56 | 34.86 |
| 12 sshash-bigreedy-ms | unitigs | 5.74 | 0.61 | 58.92 | 323 | 213 | 59.84 | 37.19 | 51.62 | 40.14 |
| Δ vs 02 | vs unitigs | +12.5% | +0.0% | -0.6% | +60.8% | +84.0% | -85.2% | +15.6% | +18.5% | +15.1% |
| 02 sshash | simplitigs | 5.08 | 0.28 | 64.09 | 209 | 115 | 360 | 32.12 | 46.80 | 34.18 |
| 12 sshash-bigreedy-ms | simplitigs | 5.60 | 0.28 | 64.16 | 324 | 219 | 60.32 | 36.68 | 50.42 | 40.86 |
| Δ vs 02 | vs simplitigs | +10.2% | +0.0% | +0.1% | +54.9% | +90.8% | -83.3% | +14.2% | +7.7% | +19.6% |
| 12 sshash-bigreedy-ms | ms | 5.71 | 0.30 | 67.61 | 295 | 191 | 61.97 | 35.86 | 50.98 | 44.03 |
| Δ vs 02 | vs simplitigs | +12.3% | +7.1% | +5.5% | +41.0% | +65.9% | -82.8% | +11.7% | +8.9% | +28.8% |
| 03 sshash-rs | unitigs | 5.05 | 0.92 | 124 | 301 | 149 | – | 30.82 | 45.26 | 34.83 |
| 13 sshash-rs-bigreedy-ms | unitigs | 5.62 | 0.70 | 125 | 267 | 251 | – | 35.15 | 49.89 | 41.18 |
| Δ vs 03 | vs unitigs | +11.3% | -23.9% | +0.6% | -11.3% | +68.7% | – | +14.0% | +10.2% | +18.2% |
| 03 sshash-rs | simplitigs | 5.07 | 0.49 | 124 | 302 | 149 | – | 31.56 | 38.88 | 33.80 |
| 13 sshash-rs-bigreedy-ms | simplitigs | 5.52 | 0.47 | 122 | 255 | 213 | – | 34.26 | 49.87 | 38.59 |
| Δ vs 03 | vs simplitigs | +8.9% | -4.1% | -1.2% | -15.4% | +43.1% | – | +8.6% | +28.3% | +14.2% |
| 13 sshash-rs-bigreedy-ms | ms | 5.63 | 0.47 | 126 | 249 | 204 | – | 32.48 | 48.47 | 37.33 |
| Δ vs 03 | vs simplitigs | +11.0% | -4.1% | +1.9% | -17.3% | +37.0% | – | +2.9% | +24.7% | +10.5% |

All indexes report identical positive k-mer counts on every streaming workload: high 662,056, low 0, mixed 662,056.
