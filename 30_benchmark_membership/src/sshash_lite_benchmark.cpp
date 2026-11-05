/*
    SSHash-Lite (01_sshash-lite / 11_sshash-lite-bigreedy-ms) port of the reference harness
    00_kmer_sets_benchmark/src/sshash_benchmark.cpp: same loops, constants, timers and output keys.

    Differences, all forced by SSHash-Lite being a membership-only dictionary:
    - no k-mer ids, hence no access benchmark, and positive queries cannot be sampled from the
      index: they are read from the queries.txt file written by the reference harness (as its
      FMSI and SBWT benchmarks do);
    - lookup is is_member();
    - streaming queries use SSHash-Lite's streaming_query_{canonical,regular}_parsing, queried
      on every k-mer of every read (no early exit against a threshold, unlike `sshash-lite
      query`). Invalid k-mers (non-ACGT) are answered as negative; they are not counted apart.
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

void perf_test_lookup(dictionary const& index, std::string const& positive_queries_filename,
                      essentials::json_lines& perf_stats)  //
{
    const uint64_t num_queries = bench::num_queries();
    constexpr uint64_t runs = 5;
    const uint64_t k = index.k();
    std::string kmer(k, 0);

    {
        // perf test positive lookup
        std::vector<std::string> lookup_queries;
        lookup_queries.reserve(num_queries);
        {
            std::ifstream in(positive_queries_filename);
            if (!in.good()) {
                throw std::runtime_error("error in opening the file '" +
                                         positive_queries_filename + "'");
            }
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                if (line.size() != k) {
                    throw std::runtime_error("query k-mer of length " +
                                             std::to_string(line.size()) + " != k");
                }
                lookup_queries.push_back(line);
            }
        }

        uint64_t num_positive_kmers = 0;
        bench::timer_type t;
        t.start();
        for (uint64_t r = 0; r != runs; ++r) {
            for (auto const& string : lookup_queries) {
                num_positive_kmers += index.is_member(string.c_str());
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
                bool res = index.is_member(string.c_str());
                essentials::do_not_optimize_away(res);
            }
        }
        t.stop();
        double nanosec_per_lookup = t.elapsed() / (runs * lookup_queries.size());
        std::cout << "negative lookup (avg_nanosec_per_kmer) " << nanosec_per_lookup << std::endl;
        perf_stats.add("negative lookup (avg_nanosec_per_kmer)", nanosec_per_lookup);
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
                num_positive_kmers += query.is_member(kmer);
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
    /* The cmd_line_parser of SSHash-Lite predates required options: the last argument of add()
       is "is boolean", and -i is checked by hand. */
    parser.add("index_filename", "SSHash-Lite index filename.", "-i", false);
    parser.add("query_filename", "Must be a fastq file compressed with gzip (extension fastq.gz).",
               "-q", false);
    parser.add("positive_queries_filename",
               "File with one positive k-mer per line, as written by the reference harness. "
               "Default: queries.txt.",
               "-p", false);
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
    std::string positive_queries_filename("queries.txt");
    if (parser.parsed("positive_queries_filename")) {
        positive_queries_filename = parser.get<std::string>("positive_queries_filename");
    }

    essentials::logger("loading index...");
    dictionary index;
    uint64_t num_bytes = essentials::load(index, index_filename.c_str());
    essentials::logger("DONE");

    essentials::json_lines perf_stats;
    perf_stats.add("index_filename", index_filename.c_str());
    perf_stats.add("k", index.k());
    perf_stats.add("m", index.m());
    perf_stats.add("canonical", index.canonicalized() ? "true" : "false");
    perf_stats.add("num_kmers", index.size());
    perf_stats.add("bits_per_kmer", (num_bytes * 8.0) / index.size());

    if (query_filename.empty()) {
        perf_test_lookup(index, positive_queries_filename, perf_stats);
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
