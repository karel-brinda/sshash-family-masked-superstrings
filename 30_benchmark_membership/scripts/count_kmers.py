"""Print the number of k-mers represented by SPSS files (one sequence per record) or by a
masked superstring (uppercase = start of a represented k-mer); exit 1 if the counts differ."""

import sys

LOWER = b"acgtn"
UPPER = b"ACGTN"


def count(path, k):
    """(num_kmers, num_strings, num_bases); streams the file, sequences may be wrapped."""
    num_strings = num_bases = num_upper = spss_kmers = 0
    masked = False
    cur = 0  # length of the current record

    def end_record():
        nonlocal spss_kmers
        if cur >= k:
            spss_kmers += cur - k + 1

    with open(path, "rb") as f:
        for line in f:
            if line.startswith(b">"):
                end_record()
                num_strings += 1
                cur = 0
                continue
            line = line.rstrip(b"\r\n")
            n = len(line)
            cur += n
            num_bases += n
            upper = n - len(line.translate(None, UPPER))
            num_upper += upper
            masked = masked or upper != n
        end_record()
    return (num_upper if masked else spss_kmers), num_strings, num_bases


k = int(sys.argv[1])
print("file\tnum_kmers\tnum_strings\tnum_bases")
counts = set()
for path in sys.argv[2:]:
    n, s, b = count(path, k)
    counts.add(n)
    print(f"{path}\t{n}\t{s}\t{b}")
sys.exit(len(counts) != 1)
