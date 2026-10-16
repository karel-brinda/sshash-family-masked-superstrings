"""Interleave the reads of two gzipped FASTQ files 1:1 (until the shorter one ends) into a
gzipped FASTQ: the mixed-hit workload from a high-hit and a low-hit read set."""

import gzip
import sys


def reads(path):
    with gzip.open(path, "rt") as f:
        while True:
            read = [f.readline() for _ in range(4)]
            if not read[0]:
                return
            yield "".join(read)


a, b, out = sys.argv[1:]
with gzip.open(out, "wt", compresslevel=6) as f:
    for ra, rb in zip(reads(a), reads(b)):
        f.write(ra)
        f.write(rb)
