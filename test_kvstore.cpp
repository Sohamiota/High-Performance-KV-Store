#include <gtest/gtest.h>
#include "kvstore.h"
#include <thread>
#include <vector>
#include <random>

class KVStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = std::make_unique<kvstore::KVStore>(100);
    }
    
    void TearDown() override {
        store.reset();
    }
    
    std::unique_ptr<kvstore::KVStore> store;
};

TEST_F(KVStoreTest, BasicOperations) {
    // Test put and get
    store->put("key1", "value1");
    
    std::string value;
    ASSERT_TRUE(store->get("key1", value));
    EXPECT_EQ(value, "value1");
    
    // Test non-existent key
    ASSERT_FALSE(store->get("nonexistent", value));
    
    // Test update
    store->put("key1", "updated_value");
    ASSERT_TRUE(store->get("key1", value));
    EXPECT_EQ(value, "updated_value");
}

TEST_F(KVStoreTest, RemoveOperation) {
    store->put("key1", "value1");
    store->put("key2", "value2");
    
    ASSERT_TRUE(store->remove("key1"));
    ASSERT_FALSE(store->remove("key1")); // Already removed
    
    std::string value;
    ASSERT_FALSE(store->get("key1", value));
    ASSERT_TRUE(store->get("key2", value));
    EXPECT_EQ(value, "value2");
}

TEST_F(KVStoreTest, LRUEviction) {
    // Create a small cache
    auto small_store = std::make_unique<kvstore::KVStore>(3);
    
    // Fill cache
    small_store->put("key1", "value1");
    small_store->put("key2", "value2");
    small_store->put("key3", "value3");
    
    EXPECT_EQ(small_store->size(), 3);
    
    // Add one more to trigger eviction
    small_store->put("key4", "value4");
    EXPECT_EQ(small_store->size(), 3);
    
    // key1 should be evicted (least recently used)
    std::string value;
    ASSERT_FALSE(small_store->get("key1", value));
    ASSERT_TRUE(small_store->get("key4", value));
}

TEST_F(KVStoreTest, ClearOperation) {
    store->put("key1", "value1");
    store->put("key2", "value2");
    
    EXPECT_EQ(store->size(), 2);
    EXPECT_FALSE(store->empty());
    
    store->clear();
    
    EXPECT_EQ(store->size(), 0);
    EXPECT_TRUE(store->empty());
    
    std::string value;
    ASSERT_FALSE(store->get("key1", value));
    ASSERT_FALSE(store->get("key2", value));
}

TEST_F(KVStoreTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    // Launch multiple threads
    for (int i = 0; i &lt; num_threads; ++i) {
        threads.emplace_back([this, i, operations_per_thread, &success_count]() {
            for (int j = 0; j &lt; operations_per_thread; ++j) {
                std::string key = "thread_" + std::to_string(i) + "_key_" + std::to_string(j);
                std::string value = "value_" + std::to_string(j);
                
                store->put(key, value);
                
                std::string retrieved_value;
                if (store->get(key, retrieved_value) && retrieved_value == value) {
                    success_count++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All operations should succeed
    EXPECT_EQ(success_count.load(), num_threads * operations_per_thread);
}

TEST_F(KVStoreTest, SnapshotOperations) {
    const std::string snapshot_file = "test_snapshot.dat";
    
    // Create store with snapshot file
    auto snapshot_store = std::make_unique<kvstore::KVStore>(100, snapshot_file);
    
    // Add some data
    snapshot_store->put("persistent_key1", "persistent_value1");
    snapshot_store->put("persistent_key2", "persistent_value2");
    
    // Save snapshot
    snapshot_store->save_snapshot();
    
    // Create new store and load snapshot
    auto new_store = std::make_unique<kvstore::KVStore>(100, snapshot_file);
    ASSERT_TRUE(new_store->load_snapshot());
    
    // Verify data was loaded
    std::string value;
    ASSERT_TRUE(new_store->get("persistent_key1", value));
    EXPECT_EQ(value, "persistent_value1");
    
    ASSERT_TRUE(new_store->get("persistent_key2", value));
    EXPECT_EQ(value, "persistent_value2");
    
    // Clean up
    std::remove(snapshot_file.c_str());
}

TEST_F(KVStoreTest, PerformanceMetrics) {
    // Perform some operations
    store->put("key1", "value1");
    store->put("key2", "value2");
    
    std::string value;
    store->get("key1", value);  // Hit
    store->get("key3", value);  // Miss
    
    const auto& metrics = store->get_metrics();
    
    EXPECT_EQ(metrics.total_operations.load(), 4);
    EXPECT_EQ(metrics.cache_hits.load(), 1);
    EXPECT_EQ(metrics.cache_misses.load(), 1);
    EXPECT_GT(metrics.hit_rate(), 0.0);
    EXPECT_LT(metrics.hit_rate(), 1.0);
}

TEST_F(KVStoreTest, StressTest) {
    const int num_operations = 10000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> key_dis(1, 1000);
    std::uniform_int_distribution<> op_dis(1, 3);
    
    for (int i = 0; i &lt; num_operations; ++i) {
        std::string key = "stress_key_" + std::to_string(key_dis(gen));
        int operation = op_dis(gen);
        
        switch (operation) {
            case 1: // Put
                store->put(key, "stress_value_" + std::to_string(i));
                break;
            case 2: // Get
                {
                    std::string value;
                    store->get(key, value);
                }
                break;
            case 3: // Remove
                store->remove(key);
                break;
        }
    }
    
    // Store should still be functional
    store->put("final_key", "final_value");
    std::string value;
    ASSERT_TRUE(store->get("final_key", value));
    EXPECT_EQ(value, "final_value");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
