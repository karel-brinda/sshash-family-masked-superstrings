//! Rust port of the reference harness 00_kmer_sets_benchmark/src/sshash_benchmark.cpp for
//! 03_sshash-rs / 13_sshash-rs-bigreedy-ms: same loops, constants and output keys.
//!
//! Differences, forced by the sshash-rs API:
//! - sshash-rs has no `access(kmer_id) -> k-mer`, hence no access benchmark, and positive
//!   queries are read from the queries.txt file written by the reference harness;
//! - lookup is `Dictionary::query`, which (like C++ `lookup`) searches both strands and returns
//!   the k-mer id; ASCII-to-k-mer encoding is inside the timed loop, as in C++;
//! - negative queries are random k-mers from a fixed-seed LCG instead of `rand()`.

use std::fs::File;
use std::hint::black_box;
use std::io::{BufRead, BufReader};
use std::time::Instant;

use flate2::read::MultiGzDecoder;
use sshash_lib::streaming_query::StreamingQueryEngine;
use sshash_lib::{Dictionary, Kmer, KmerBits};

/// Number of random queries: $BENCH_NUM_QUERIES, or 1M as in the reference harness.
fn num_queries() -> usize {
    std::env::var("BENCH_NUM_QUERIES").map_or(1_000_000, |n| n.parse().expect("BENCH_NUM_QUERIES"))
}
const RUNS: usize = 5;

/// One JSON line on stderr, like `essentials::json_lines` (all values are strings).
struct JsonLine(Vec<(String, String)>);

impl JsonLine {
    fn add(&mut self, name: &str, value: impl ToString) {
        self.0.push((name.to_string(), value.to_string()));
    }
    fn print(&self) {
        let fields: Vec<String> = self.0.iter().map(|(n, v)| format!("\"{n}\": \"{v}\"")).collect();
        eprintln!("{{{}}}", fields.join(", "));
    }
}

fn perf_test_lookup<const K: usize>(dict: &Dictionary, positive_queries: &str, stats: &mut JsonLine)
where
    Kmer<K>: KmerBits,
{
    {
        // perf test positive lookup
        let lookup_queries: Vec<Vec<u8>> = BufReader::new(
            File::open(positive_queries)
                .unwrap_or_else(|e| panic!("error in opening the file '{positive_queries}': {e}")),
        )
        .lines()
        .map(|l| l.unwrap().into_bytes())
        .filter(|l| !l.is_empty())
        .inspect(|l| assert_eq!(l.len(), K, "query k-mer of length {} != k", l.len()))
        .collect();

        let mut num_positive_kmers: u64 = 0;
        let t = Instant::now();
        for _ in 0..RUNS {
            for string in &lookup_queries {
                let kmer = Kmer::<K>::from_ascii_unchecked(string);
                num_positive_kmers += dict.query(&kmer).is_found() as u64;
            }
        }
        let elapsed = t.elapsed().as_nanos() as f64;
        println!("num_positive_kmers = {num_positive_kmers}");
        let nanosec_per_lookup = elapsed / (RUNS * lookup_queries.len()) as f64;
        println!("positive lookup (avg_nanosec_per_kmer) = {nanosec_per_lookup}");
        stats.add("num_positive_queries", lookup_queries.len());
        stats.add("num_positive_kmers", num_positive_kmers / RUNS as u64);
        stats.add("positive lookup (avg_nanosec_per_kmer)", nanosec_per_lookup);
    }

    {
        // perf test negative lookup
        let mut rng: u64 = 12345;
        let lookup_queries: Vec<Vec<u8>> = (0..num_queries())
            .map(|_| {
                (0..K)
                    .map(|_| {
                        rng = rng.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
                        b"ACGT"[(rng >> 62) as usize]
                    })
                    .collect()
            })
            .collect();
        let t = Instant::now();
        for _ in 0..RUNS {
            for string in &lookup_queries {
                let kmer = Kmer::<K>::from_ascii_unchecked(string);
                black_box(dict.query(&kmer).kmer_id);
            }
        }
        let elapsed = t.elapsed().as_nanos() as f64;
        let nanosec_per_lookup = elapsed / (RUNS * lookup_queries.len()) as f64;
        println!("negative lookup (avg_nanosec_per_kmer) {nanosec_per_lookup}");
        stats.add("negative lookup (avg_nanosec_per_kmer)", nanosec_per_lookup);
    }
}

fn streaming_query_from_fastq_file<const K: usize>(dict: &Dictionary, stats: &mut JsonLine, query_filename: &str)
where
    Kmer<K>: KmerBits,
{
    let file = File::open(query_filename)
        .unwrap_or_else(|e| panic!("error in opening the file '{query_filename}': {e}"));

    let t = Instant::now();
    let mut is = BufReader::new(MultiGzDecoder::new(file));
    let mut total_num_kmers: u64 = 0;
    let mut num_positive_kmers: u64 = 0;
    let mut line = String::new();
    let mut engine = StreamingQueryEngine::<K>::new(dict);
    loop {
        engine.reset();
        /* We assume the file is well-formed, i.e., there are exactly 4 lines per read. */
        line.clear();
        if is.read_line(&mut line).unwrap() == 0 {
            break; // skip first header line
        }
        line.clear();
        is.read_line(&mut line).unwrap();
        let read = line.trim_end().as_bytes();
        if read.len() >= K {
            let num_kmers = read.len() - K + 1;
            total_num_kmers += num_kmers as u64;
            for i in 0..num_kmers {
                num_positive_kmers += engine.lookup(&read[i..i + K]).is_found() as u64;
            }
        }
        line.clear();
        is.read_line(&mut line).unwrap(); // skip '+'
        line.clear();
        is.read_line(&mut line).unwrap(); // skip score
    }
    let elapsed = t.elapsed();

    stats.add("total_num_kmers", total_num_kmers);
    stats.add("num_positive_kmers", num_positive_kmers);
    stats.add("num_negative_kmers", total_num_kmers - num_positive_kmers);
    stats.add("num_searches", engine.num_searches());
    stats.add("num_extensions", engine.num_extensions());
    stats.add("elapsed_ms", elapsed.as_secs_f64() * 1e3);
    stats.add("avg_nanosec_per_kmer", elapsed.as_nanos() as f64 / total_num_kmers as f64);
}

fn usage() -> ! {
    eprintln!("usage: bench -i <index> [-q <reads.fastq.gz>] [-p <queries.txt>]");
    std::process::exit(1);
}

fn main() {
    let mut index_filename = None;
    let mut query_filename = None;
    let mut positive_queries = "queries.txt".to_string();
    let mut args = std::env::args().skip(1);
    while let Some(a) = args.next() {
        let v = args.next().unwrap_or_else(|| usage());
        match a.as_str() {
            "-i" => index_filename = Some(v),
            "-q" => query_filename = Some(v),
            "-p" => positive_queries = v,
            _ => usage(),
        }
    }
    let index_filename = index_filename.unwrap_or_else(|| usage());

    eprintln!("loading index...");
    let dict = Dictionary::load(&index_filename).expect("cannot load index");
    eprintln!("DONE");

    let mut stats = JsonLine(Vec::new());
    stats.add("index_filename", &index_filename);
    stats.add("k", dict.k());
    stats.add("m", dict.m());

    sshash_lib::dispatch_on_k!(dict.k(), K => {
        match &query_filename {
            None => perf_test_lookup::<K>(&dict, &positive_queries, &mut stats),
            Some(q) => {
                stats.add("query_filename", q);
                streaming_query_from_fastq_file::<K>(&dict, &mut stats, q);
            }
        }
    });

    stats.print();
}
