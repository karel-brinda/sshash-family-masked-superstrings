"""Print the number of k-mers represented by SPSS files (one sequence per record) or by a
masked superstring (uppercase = start of a represented k-mer); exit 1 if the counts differ."""

import sys


def records(path):
    seq = []
    with open(path) as f:
        for line in f:
            if line.startswith(">"):
                if seq:
                    yield "".join(seq)
                seq = []
            else:
                seq.append(line.strip())
    if seq:
        yield "".join(seq)


def count(path, k):
    seqs = list(records(path))
    if any(c.islower() for s in seqs for c in s):  # masked superstring
        return sum(c.isupper() for s in seqs for c in s), len(seqs), sum(map(len, seqs))
    return sum(len(s) - k + 1 for s in seqs if len(s) >= k), len(seqs), sum(map(len, seqs))


k = int(sys.argv[1])
print("file\tnum_kmers\tnum_strings\tnum_bases")
counts = set()
for path in sys.argv[2:]:
    n, s, b = count(path, k)
    counts.add(n)
    print(f"{path}\t{n}\t{s}\t{b}")
sys.exit(len(counts) != 1)
