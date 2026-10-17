ecoli_pangenome_661k_r0.1, k=31, 55,095,014 k-mers; medians of 3 runs; Δ = change vs the 0x original

| tool | input | bits/kmer | build (s) | build RSS (MB) | pos. lookup (ns) | neg. lookup (ns) | access (ns) | stream high (ns/kmer) | stream low (ns/kmer) | stream mixed (ns/kmer) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01 sshash-lite | unitigs | 94.86 | 48.79 | 2451 | 2543 | 1523 | – | 804 | 321 | 676 |
| 11 sshash-lite-bigreedy-ms | unitigs | 94.86 | 48.96 | 2614 | 2075 | 1328 | – | 684 | 294 | 582 |
| Δ vs 01 | vs unitigs | +0.0% | +0.3% | +6.6% | -18.4% | -12.8% | – | -15.0% | -8.4% | -13.9% |
| 01 sshash-lite | simplitigs | 94.79 | 48.98 | 2419 | 1625 | 1416 | – | 810 | 329 | 681 |
| 11 sshash-lite-bigreedy-ms | simplitigs | 94.79 | 48.95 | 2502 | 2304 | 1462 | – | 681 | 294 | 579 |
| Δ vs 01 | vs simplitigs | +0.0% | -0.1% | +3.5% | +41.8% | +3.3% | – | -15.9% | -10.8% | -15.0% |
| 11 sshash-lite-bigreedy-ms | ms | 52.41 | 41.91 | 3092 | 1454 | 1208 | – | 719 | 293 | 605 |
| Δ vs 01 | vs simplitigs | -44.7% | -14.4% | +27.8% | -10.5% | -14.7% | – | -11.3% | -11.1% | -11.2% |
| 02 sshash | unitigs | 104 | 25.45 | 3005 | 1024 | 442 | 1857 | 259 | 115 | 216 |
| 12 sshash-bigreedy-ms | unitigs | 140 | 27.49 | 3465 | 2093 | 944 | 767 | 430 | 168 | 356 |
| Δ vs 02 | vs unitigs | +35.2% | +8.0% | +15.3% | +104.4% | +113.6% | -58.7% | +66.1% | +45.8% | +64.6% |
| 02 sshash | simplitigs | 104 | 25.23 | 2952 | 1053 | 471 | 1908 | 258 | 124 | 219 |
| 12 sshash-bigreedy-ms | simplitigs | 140 | 26.42 | 3530 | 1748 | 877 | 755 | 439 | 170 | 362 |
| Δ vs 02 | vs simplitigs | +35.2% | +4.7% | +19.6% | +66.0% | +86.4% | -60.4% | +70.1% | +37.6% | +65.6% |
| 12 sshash-bigreedy-ms | ms | 97.78 | 19.81 | 3752 | 1706 | 831 | 725 | 495 | 170 | 405 |
| Δ vs 02 | vs simplitigs | -5.7% | -21.5% | +27.1% | +62.0% | +76.7% | -62.0% | +91.7% | +37.8% | +84.9% |
| 03 sshash-rs | unitigs | 94.41 | 29.76 | 4267 | 1457 | 619 | – | 218 | 150 | 194 |
| 13 sshash-rs-bigreedy-ms | unitigs | 125 | 31.21 | 5520 | 4155 | 1242 | – | 381 | 189 | 331 |
| Δ vs 03 | vs unitigs | +32.6% | +4.9% | +29.3% | +185.1% | +100.6% | – | +74.6% | +26.2% | +70.0% |
| 03 sshash-rs | simplitigs | 94.34 | 29.67 | 3953 | 2078 | 748 | – | 225 | 151 | 195 |
| 13 sshash-rs-bigreedy-ms | simplitigs | 125 | 30.41 | 4806 | 2324 | 1066 | – | 406 | 233 | 344 |
| Δ vs 03 | vs simplitigs | +32.6% | +2.5% | +21.6% | +11.8% | +42.7% | – | +80.7% | +54.6% | +76.6% |
| 13 sshash-rs-bigreedy-ms | ms | 82.78 | 19.03 | 4130 | 1532 | 885 | – | 477 | 191 | 394 |
| Δ vs 03 | vs simplitigs | -12.3% | -35.9% | +4.5% | -26.3% | +18.4% | – | +112.3% | +26.8% | +102.3% |

All indexes report identical positive k-mer counts on every streaming workload: high 109,264, low 1,195, mixed 110,459.
