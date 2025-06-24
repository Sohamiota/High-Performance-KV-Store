#include "kvstore.h"
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>
#include <numeric>

class Benchmark {
private:
    kvstore::KVStore& store_;
    std::atomic<bool> stop_flag_{false};
    std::atomic<uint64_t> operations_completed_{0};
    
    std::string generate_random_string(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        static thread_local std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
        
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i &lt; length; ++i) {
            result += charset[dis(gen)];
        }
        return result;
    }
    
    void worker_thread(int thread_id, int num_operations, double read_ratio) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> op_dis(0.0, 1.0);
        std::uniform_int_distribution<> key_dis(1, 10000);
        
        for (int i = 0; i &lt; num_operations && !stop_flag_; ++i) {
            std::string key = "key_" + std::to_string(key_dis(gen));
            
            if (op_dis(gen) &lt; read_ratio) {
                // Read operation
                std::string value;
                store_.get(key, value);
            } else {
                // Write operation
                std::string value = generate_random_string(50);
                store_.put(key, value);
            }
            
            operations_completed_++;
        }
    }
    
public:
    explicit Benchmark(kvstore::KVStore& store) : store_(store) {}
    
    void run_concurrent_benchmark(int num_threads, int operations_per_thread, double read_ratio) {
        std::cout &lt;&lt; "Running concurrent benchmark:\n"
                  &lt;&lt; "  Threads: " &lt;&lt; num_threads &lt;&lt; "\n"
                  &lt;&lt; "  Operations per thread: " &lt;&lt; operations_per_thread &lt;&lt; "\n"
                  &lt;&lt; "  Read ratio: " &lt;&lt; (read_ratio * 100) &lt;&lt; "%\n\n";
        
        // Reset metrics
        store_.clear();
        operations_completed_ = 0;
        stop_flag_ = false;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Launch worker threads
        std::vector<std::thread> threads;
        for (int i = 0; i &lt; num_threads; ++i) {
            threads.emplace_back(&Benchmark::worker_thread, this, i, operations_per_thread, read_ratio);
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Print results
        uint64_t total_ops = operations_completed_.load();
        double ops_per_second = static_cast<double>(total_ops) / (duration.count() / 1000.0);
        
        const auto& metrics = store_.get_metrics();
        
        std::cout &lt;&lt; "Benchmark Results:\n"
                  &lt;&lt; "  Total operations: " &lt;&lt; total_ops &lt;&lt; "\n"
                  &lt;&lt; "  Duration: " &lt;&lt; duration.count() &lt;&lt; " ms\n"
                  &lt;&lt; "  Operations/sec: " &lt;&lt; std::fixed &lt;&lt; std::setprecision(2) &lt;&lt; ops_per_second &lt;&lt; "\n"
                  &lt;&lt; "  Cache hit rate: " &lt;&lt; std::fixed &lt;&lt; std::setprecision(2) 
                  &lt;&lt; (metrics.hit_rate() * 100) &lt;&lt; "%\n"
                  &lt;&lt; "  Final cache size: " &lt;&lt; store_.size() &lt;&lt; "\n"
                  &lt;&lt; "  Evictions: " &lt;&lt; metrics.evictions &lt;&lt; "\n\n";
    }
    
    void run_latency_test(int num_operations) {
        std::cout &lt;&lt; "Running latency test with " &lt;&lt; num_operations &lt;&lt; " operations...\n";
        
        std::vector<double> latencies;
        latencies.reserve(num_operations);
        
        // Warm up
        for (int i = 0; i &lt; 1000; ++i) {
            store_.put("warmup_" + std::to_string(i), "value");
        }
        
        // Measure latencies
        for (int i = 0; i &lt; num_operations; ++i) {
            std::string key = "latency_test_" + std::to_string(i);
            std::string value = generate_random_string(100);
            
            auto start = std::chrono::high_resolution_clock::now();
            store_.put(key, value);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            latencies.push_back(latency / 1000.0); // Convert to microseconds
        }
        
        // Calculate statistics
        std::sort(latencies.begin(), latencies.end());
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double p50 = latencies[latencies.size() * 0.5];
        double p95 = latencies[latencies.size() * 0.95];
        double p99 = latencies[latencies.size() * 0.99];
        
        std::cout &lt;&lt; "Latency Results (microseconds):\n"
                  &lt;&lt; "  Average: " &lt;&lt; std::fixed &lt;&lt; std::setprecision(2) &lt;&lt; avg &lt;&lt; "\n"
                  &lt;&lt; "  P50: " &lt;&lt; p50 &lt;&lt; "\n"
                  &lt;&lt; "  P95: " &lt;&lt; p95 &lt;&lt; "\n"
                  &lt;&lt; "  P99: " &lt;&lt; p99 &lt;&lt; "\n"
                  &lt;&lt; "  Min: " &lt;&lt; latencies.front() &lt;&lt; "\n"
                  &lt;&lt; "  Max: " &lt;&lt; latencies.back() &lt;&lt; "\n\n";
    }
};

int main(int argc, char* argv[]) {
    size_t capacity = 10000;
    int num_threads = std::thread::hardware_concurrency();
    int operations_per_thread = 10000;
    double read_ratio = 0.8;
    
    // Parse command line arguments
    for (int i = 1; i &lt; argc; i++) {
        std::string arg = argv[i];
        if (arg == "--capacity" && i + 1 &lt; argc) {
            capacity = std::stoul(argv[++i]);
        } else if (arg == "--threads" && i + 1 &lt; argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "--operations" && i + 1 &lt; argc) {
            operations_per_thread = std::stoi(argv[++i]);
        } else if (arg == "--read-ratio" && i + 1 &lt; argc) {
            read_ratio = std::stod(argv[++i]);
        } else if (arg == "--help") {
            std::cout &lt;&lt; "Usage: " &lt;&lt; argv[0] &lt;&lt; " [options]\n"
                      &lt;&lt; "Options:\n"
                      &lt;&lt; "  --capacity <size>     Set cache capacity (default: 10000)\n"
                      &lt;&lt; "  --threads <count>     Set number of threads (default: hardware concurrency)\n"
                      &lt;&lt; "  --operations <count>  Set operations per thread (default: 10000)\n"
                      &lt;&lt; "  --read-ratio <ratio>  Set read operation ratio 0.0-1.0 (default: 0.8)\n"
                      &lt;&lt; "  --help                Show this help\n";
            return 0;
        }
    }
    
    try {
        kvstore::KVStore store(capacity);
        Benchmark benchmark(store);
        
        std::cout &lt;&lt; "KVStore Performance Benchmark\n";
        std::cout &lt;&lt; "=============================\n\n";
        
        // Run concurrent benchmark
        benchmark.run_concurrent_benchmark(num_threads, operations_per_thread, read_ratio);
        
        // Run latency test
        benchmark.run_latency_test(10000);
        
    } catch (const std::exception& e) {
        std::cerr &lt;&lt; "Benchmark failed: " &lt;&lt; e.what() &lt;&lt; std::endl;
        return 1;
    }
    
    return 0;
}
