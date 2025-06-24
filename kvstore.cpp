#include "kvstore.h"
#include <iostream>

namespace kvstore {

KVStore::KVStore(size_t capacity, const std::string& snapshot_file)
    : cache_(std::make_unique<LRUCache>(capacity)), snapshot_file_(snapshot_file) {
    
    if (!snapshot_file_.empty()) {
        load_snapshot();
    }
}

KVStore::~KVStore() {
    if (!snapshot_file_.empty()) {
        try {
            save_snapshot();
        } catch (const std::exception& e) {
            std::cerr &lt;&lt; "Failed to save snapshot on destruction: " &lt;&lt; e.what() &lt;&lt; std::endl;
        }
    }
}

bool KVStore::get(const std::string& key, std::string& value) {
    metrics_.total_operations++;
    
    bool found = cache_->get(key, value);
    if (found) {
        metrics_.cache_hits++;
    } else {
        metrics_.cache_misses++;
    }
    
    return found;
}

void KVStore::put(const std::string& key, const std::string& value) {
    metrics_.total_operations++;
    
    size_t old_size = cache_->size();
    cache_->put(key, value);
    
    // Check if eviction occurred
    if (cache_->size() &lt; old_size + 1) {
        metrics_.evictions++;
    }
}

bool KVStore::remove(const std::string& key) {
    metrics_.total_operations++;
    return cache_->remove(key);
}

void KVStore::clear() {
    cache_->clear();
    reset_metrics();
}

void KVStore::save_snapshot() const {
    if (!snapshot_file_.empty()) {
        cache_->save_snapshot(snapshot_file_);
    }
}

bool KVStore::load_snapshot() {
    if (!snapshot_file_.empty()) {
        return cache_->load_snapshot(snapshot_file_);
    }
    return false;
}

void KVStore::reset_metrics() {
    metrics_ = PerformanceMetrics();
}

size_t KVStore::size() const {
    return cache_->size();
}

bool KVStore::empty() const {
    return cache_->empty();
}

} // namespace kvstore
