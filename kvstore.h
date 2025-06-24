#pragma once

#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <fstream>

namespace kvstore {

struct CacheEntry {
    std::string value;
    std::chrono::steady_clock::time_point last_accessed;
    size_t access_count;
    
    CacheEntry(const std::string& val) 
        : value(val), last_accessed(std::chrono::steady_clock::now()), access_count(1) {}
};

class LRUCache {
private:
    struct Node {
        std::string key;
        std::shared_ptr<CacheEntry> entry;
        std::shared_ptr<Node> prev, next;
        
        Node(const std::string& k, std::shared_ptr<CacheEntry> e) 
            : key(k), entry(e) {}
    };
    
    using NodePtr = std::shared_ptr<Node>;
    
    std::unordered_map<std::string, NodePtr> cache_map;
    NodePtr head, tail;
    size_t capacity;
    size_t current_size;
    mutable std::shared_mutex mutex_;
    
    void move_to_front(NodePtr node);
    void remove_node(NodePtr node);
    NodePtr remove_tail();
    
public:
    explicit LRUCache(size_t cap);
    
    bool get(const std::string& key, std::string& value);
    void put(const std::string& key, const std::string& value);
    bool remove(const std::string& key);
    void clear();
    size_t size() const;
    bool empty() const;
    
    // Snapshot operations
    void save_snapshot(const std::string& filename) const;
    bool load_snapshot(const std::string& filename);
};

struct PerformanceMetrics {
    std::atomic<uint64_t> total_operations{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    std::atomic<uint64_t> evictions{0};
    std::chrono::steady_clock::time_point start_time;
    
    PerformanceMetrics() : start_time(std::chrono::steady_clock::now()) {}
    
    double hit_rate() const {
        uint64_t hits = cache_hits.load();
        uint64_t total = hits + cache_misses.load();
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
    
    double operations_per_second() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        return duration.count() > 0 ? static_cast<double>(total_operations.load()) / duration.count() : 0.0;
    }
};

class KVStore {
private:
    std::unique_ptr<LRUCache> cache_;
    mutable PerformanceMetrics metrics_;
    std::string snapshot_file_;
    
public:
    explicit KVStore(size_t capacity, const std::string& snapshot_file = "");
    ~KVStore();
    
    // Core operations
    bool get(const std::string& key, std::string& value);
    void put(const std::string& key, const std::string& value);
    bool remove(const std::string& key);
    void clear();
    
    // Persistence
    void save_snapshot() const;
    bool load_snapshot();
    
    // Metrics
    const PerformanceMetrics& get_metrics() const { return metrics_; }
    void reset_metrics();
    
    // Info
    size_t size() const;
    bool empty() const;
};

} // namespace kvstore
