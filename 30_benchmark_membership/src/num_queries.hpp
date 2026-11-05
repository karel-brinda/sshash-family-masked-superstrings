#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

namespace bench {

/* Number of random lookup / access queries: $BENCH_NUM_QUERIES, or 1M as in the reference
   harness. scripts/run.sh sets it per dataset (datasets.tsv). */
inline uint64_t num_queries() {
    char const* n = std::getenv("BENCH_NUM_QUERIES");
    return n ? std::stoull(n) : 1'000'000;
}

}  // namespace bench
