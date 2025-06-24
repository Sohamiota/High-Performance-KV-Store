#include "kvstore.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace kvstore {

LRUCache::LRUCache(size_t cap) : capacity(cap), current_size(0) {
    if (capacity == 0) {
        throw std::invalid_argument("Cache capacity must be greater than 0");
    }
    
    // Create dummy head and tail nodes
    head = std::make_shared<Node>("", nullptr);
    tail = std::make_shared<Node>("", nullptr);
    head->next = tail;
    tail->prev = head;
}

void LRUCache::move_to_front(NodePtr node) {
    // Remove from current position
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    
    // Insert after head
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

void LRUCache::remove_node(NodePtr node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
}

LRUCache::NodePtr LRUCache::remove_tail() {
    NodePtr last = tail->prev;
    if (last != head) {
        remove_node(last);
        return last;
    }
    return nullptr;
}

bool LRUCache::get(const std::string& key, std::string& value) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return false;
    }
    
    // Update access time and count
    it->second->entry->last_accessed = std::chrono::steady_clock::now();
    it->second->entry->access_count++;
    
    // Move to front (most recently used)
    lock.unlock();
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    move_to_front(it->second);
    
    value = it->second->entry->value;
    return true;
}

void LRUCache::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        // Update existing entry
        it->second->entry->value = value;
        it->second->entry->last_accessed = std::chrono::steady_clock::now();
        it->second->entry->access_count++;
        move_to_front(it->second);
        return;
    }
    
    // Add new entry
    auto entry = std::make_shared<CacheEntry>(value);
    auto node = std::make_shared<Node>(key, entry);
    
    if (current_size >= capacity) {
        // Evict least recently used
        NodePtr tail_node = remove_tail();
        if (tail_node) {
            cache_map.erase(tail_node->key);
            current_size--;
        }
    }
    
    // Insert new node at front
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
    
    cache_map[key] = node;
    current_size++;
}

bool LRUCache::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
        return false;
    }
    
    remove_node(it->second);
    cache_map.erase(it);
    current_size--;
    return true;
}

void LRUCache::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    cache_map.clear();
    head->next = tail;
    tail->prev = head;
    current_size = 0;
}

size_t LRUCache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return current_size;
}

bool LRUCache::empty() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return current_size == 0;
}

void LRUCache::save_snapshot(const std::string& filename) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open snapshot file for writing");
    }
    
    // Write header
    uint32_t version = 1;
    uint32_t count = static_cast<uint32_t>(current_size);
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write entries
    NodePtr current = head->next;
    while (current != tail) {
        uint32_t key_size = static_cast<uint32_t>(current->key.size());
        uint32_t value_size = static_cast<uint32_t>(current->entry->value.size());
        
        file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
        file.write(current->key.c_str(), key_size);
        file.write(reinterpret_cast<const char*>(&value_size), sizeof(value_size));
        file.write(current->entry->value.c_str(), value_size);
        
        current = current->next;
    }
}

bool LRUCache::load_snapshot(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Clear existing data
    cache_map.clear();
    head->next = tail;
    tail->prev = head;
    current_size = 0;
    
    // Read header
    uint32_t version, count;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    if (version != 1) {
        return false;
    }
    
    // Read entries
    for (uint32_t i = 0; i &lt; count && i &lt; capacity; ++i) {
        uint32_t key_size, value_size;
        file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
        
        std::string key(key_size, '\0');
        file.read(&key[0], key_size);
        
        file.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));
        std::string value(value_size, '\0');
        file.read(&value[0], value_size);
        
        // Add to cache (without lock since we already have it)
        auto entry = std::make_shared<CacheEntry>(value);
        auto node = std::make_shared<Node>(key, entry);
        
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
        
        cache_map[key] = node;
        current_size++;
    }
    
    return true;
}

} // namespace kvstore
