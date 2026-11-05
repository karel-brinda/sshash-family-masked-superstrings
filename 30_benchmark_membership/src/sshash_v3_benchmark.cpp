/*
    SSHash v3 (04_sshash-v3 / 14_sshash-v3-bigreedy-ms) port of the reference harness
    00_kmer_sets_benchmark/src/sshash_benchmark.cpp: same loops, constants, timers and output keys.
    v3 has k-mer ids, so, as in the reference, positive queries are sampled through access() and
    the access benchmark is run.

    Differences, all forced by the 2022 API (which SSHash-Lite inherited):
    - dictionary is a plain struct; lookup() returns the k-mer id; num_kmers() is size();
      canonical() is canonicalized();
    - streaming queries use streaming_query_{canonical,regular}_parsing (start() instead of
      reset(), lookup_advanced() instead of lookup()). They keep no positive/negative counters,
      so positives are counted from the answers here, and invalid k-mers (non-ACGT) are counted
      as negative, as in the SSHash-Lite harness;
    - the gz stream and the bench:: helpers live in other places than in the reference.
    Also, the number of random queries is $BENCH_NUM_QUERIES (default: 1M, as in the reference).
*/

#include <iostream>

#include "external/pthash/external/cmd_line_parser/include/parser.hpp"
#include "include/dictionary.hpp"
#include "include/gz/zip_stream.hpp"
#include "include/query/streaming_query_canonical_parsing.hpp"
#include "include/query/streaming_query_regular_parsing.hpp"
#include "num_queries.hpp"

using namespace sshash;

namespace bench {

using timer_type = essentials::timer<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;

void random_kmer(char* kmer, uint64_t k) {
    for (uint64_t i = 0; i != k; ++i) kmer[i] = "ACGT"[rand() % 4];
}

}  // namespace bench

void perf_test_lookup_access(dictionary const& index, essentials::json_lines& perf_stats)  //
{
    const uint64_t num_queries = bench::num_queries();
    constexpr uint64_t runs = 5;
    essentials::uniform_int_rng<uint64_t> distr(0, index.size() - 1,
                                                essentials::get_random_seed());
    const uint64_t k = index.k();
    std::string kmer(k, 0);
    std::string kmer_rc(k, 0);

    {
        // perf test positive lookup
        std::vector<std::string> lookup_queries;
        lookup_queries.reserve(num_queries);
        for (uint64_t i = 0; i != num_queries; ++i) {
            uint64_t id = distr.gen();
            index.access(id, kmer.data());
            if ((i & 1) == 0) {
                /* transform 50% of the kmers into their reverse complements */
                util::compute_reverse_complement(kmer.data(), kmer_rc.data(), k);
                lookup_queries.push_back(kmer_rc);
            } else {
                lookup_queries.push_back(kmer);
            }
        }

        uint64_t num_positive_kmers = 0;
        bench::timer_type t;
        t.start();
        for (uint64_t r = 0; r != runs; ++r) {
            for (auto const& string : lookup_queries) {
                uint64_t id = index.lookup(string.c_str());
                num_positive_kmers += id != constants::invalid_uint64;
            }
        }
        t.stop();
        std::cout << "num_positive_kmers = " << num_positive_kmers << std::endl;
        double nanosec_per_lookup = t.elapsed() / (runs * lookup_queries.size());
        std::cout << "positive lookup (avg_nanosec_per_kmer) = " << nanosec_per_lookup << std::endl;
        perf_stats.add("num_positive_queries", lookup_queries.size());
        perf_stats.add("num_positive_kmers", num_positive_kmers / runs);
        perf_stats.add("positive lookup (avg_nanosec_per_kmer)", nanosec_per_lookup);
    }

    {
        // perf test negative lookup
        std::vector<std::string> lookup_queries;
        lookup_queries.reserve(num_queries);
        for (uint64_t i = 0; i != num_queries; ++i) {
            bench::random_kmer(kmer.data(), k);
            lookup_queries.push_back(kmer);
        }
        bench::timer_type t;
        t.start();
        for (uint64_t r = 0; r != runs; ++r) {
            for (auto const& string : lookup_queries) {
                uint64_t id = index.lookup(string.c_str());
                essentials::do_not_optimize_away(id);
            }
        }
        t.stop();
        double nanosec_per_lookup = t.elapsed() / (runs * lookup_queries.size());
        std::cout << "negative lookup (avg_nanosec_per_kmer) " << nanosec_per_lookup << std::endl;
        perf_stats.add("negative lookup (avg_nanosec_per_kmer)", nanosec_per_lookup);
    }

    {
        // perf test access
        std::vector<uint64_t> access_queries;
        access_queries.reserve(num_queries);
        for (uint64_t i = 0; i != num_queries; ++i) access_queries.push_back(distr.gen());
        bench::timer_type t;
        t.start();
        for (uint64_t r = 0; r != runs; ++r) {
            for (auto id : access_queries) {
                index.access(id, kmer.data());
                essentials::do_not_optimize_away(kmer[0]);
            }
        }
        t.stop();
        double nanosec_per_access = t.elapsed() / static_cast<double>(runs * access_queries.size());
        std::cout << "access (avg_nanosec_per_kmer) = " << nanosec_per_access << std::endl;
        perf_stats.add("access (avg_nanosec_per_kmer)", nanosec_per_access);
    }
}

template <typename Query>
void streaming_query_from_fastq_file(dictionary const& index,              //
                                     essentials::json_lines& perf_stats,  //
                                     std::istream& is)                    //
{
    essentials::timer<std::chrono::high_resolution_clock, std::chrono::microseconds> t;

    t.start();
    uint64_t total_num_kmers = 0;
    uint64_t num_positive_kmers = 0;
    std::string line;
    const uint64_t k = index.k();
    Query query(&index);
    while (!is.eof()) {
        query.start();
        /* We assume the file is well-formed, i.e., there are exactly 4 lines per read. */
        std::getline(is, line);  // skip first header line
        std::getline(is, line);
        if (line.size() >= k) {
            const uint64_t num_kmers = line.size() - k + 1;
            total_num_kmers += num_kmers;
            for (uint64_t i = 0; i != num_kmers; ++i) {
                char const* kmer = line.data() + i;
                auto res = query.lookup_advanced(kmer);
                num_positive_kmers += res.kmer_id != constants::invalid_uint64;
            }
        }
        std::getline(is, line);  // skip '+'
        std::getline(is, line);  // skip score
    }
    t.stop();

    perf_stats.add("total_num_kmers", total_num_kmers);
    perf_stats.add("num_positive_kmers", num_positive_kmers);
    perf_stats.add("num_negative_kmers", total_num_kmers - num_positive_kmers);
    perf_stats.add("num_searches", query.num_searches());
    perf_stats.add("num_extensions", query.num_extensions());
    perf_stats.add("elapsed_ms", t.elapsed() / 1000);
    perf_stats.add("avg_nanosec_per_kmer", (t.elapsed() * 1000) / total_num_kmers);
}

int main(int argc, char** argv)  //
{
    cmd_line_parser::parser parser(argc, argv);
    /* The cmd_line_parser of v3 predates required options: the last argument of add() is
       "is boolean", and -i is checked by hand. */
    parser.add("index_filename", "SSHash index filename.", "-i", false);
    parser.add("query_filename", "Must be a fastq file compressed with gzip (extension fastq.gz).",
               "-q", false);
    if (!parser.parse()) return 0;
    if (!parser.parsed("index_filename")) {
        std::cerr << "missing -i index_filename" << std::endl;
        return 1;
    }
    auto index_filename = parser.get<std::string>("index_filename");

    std::string query_filename("");
    if (parser.parsed("query_filename")) {
        query_filename = parser.get<std::string>("query_filename");
    }

    essentials::logger("loading index...");
    dictionary index;
    essentials::load(index, index_filename.c_str());
    essentials::logger("DONE");

    essentials::json_lines perf_stats;
    perf_stats.add("index_filename", index_filename.c_str());
    perf_stats.add("k", index.k());
    perf_stats.add("m", index.m());
    perf_stats.add("canonical", index.canonicalized() ? "true" : "false");

    if (query_filename.empty()) {
        perf_test_lookup_access(index, perf_stats);
    } else  //
    {
        std::ifstream is(query_filename.c_str());
        if (!is.good()) {
            std::cerr << "error in opening the file '" + query_filename + "'" << std::endl;
            return 1;
        }
        perf_stats.add("query_filename", query_filename.c_str());
        essentials::logger("performing queries from file '" + query_filename + "'...");
        zip_istream zis(is);
        if (index.canonicalized()) {
            streaming_query_from_fastq_file<streaming_query_canonical_parsing>(index, perf_stats,
                                                                               zis);
        } else {
            streaming_query_from_fastq_file<streaming_query_regular_parsing>(index, perf_stats,
                                                                             zis);
        }
        is.close();
        essentials::logger("DONE");
    }

    perf_stats.print();

    return 0;
}
