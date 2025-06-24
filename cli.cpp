#include "kvstore.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>

class KVStoreCLI {
private:
    kvstore::KVStore store_;
    bool running_;
    
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }
    
    void print_help() {
        std::cout &lt;&lt; "Available commands:\n"
                  &lt;&lt; "  GET <key>           - Get value for key\n"
                  &lt;&lt; "  PUT <key> <value>   - Set key to value\n"
                  &lt;&lt; "  DEL <key>           - Delete key\n"
                  &lt;&lt; "  CLEAR               - Clear all entries\n"
                  &lt;&lt; "  SIZE                - Show number of entries\n"
                  &lt;&lt; "  STATS               - Show performance statistics\n"
                  &lt;&lt; "  SAVE                - Save snapshot to disk\n"
                  &lt;&lt; "  LOAD                - Load snapshot from disk\n"
                  &lt;&lt; "  HELP                - Show this help\n"
                  &lt;&lt; "  QUIT                - Exit the program\n";
    }
    
    void print_stats() {
        const auto& metrics = store_.get_metrics();
        std::cout &lt;&lt; "Performance Statistics:\n"
                  &lt;&lt; "  Total operations: " &lt;&lt; metrics.total_operations &lt;&lt; "\n"
                  &lt;&lt; "  Cache hits: " &lt;&lt; metrics.cache_hits &lt;&lt; "\n"
                  &lt;&lt; "  Cache misses: " &lt;&lt; metrics.cache_misses &lt;&lt; "\n"
                  &lt;&lt; "  Hit rate: " &lt;&lt; std::fixed &lt;&lt; std::setprecision(2) 
                  &lt;&lt; (metrics.hit_rate() * 100) &lt;&lt; "%\n"
                  &lt;&lt; "  Evictions: " &lt;&lt; metrics.evictions &lt;&lt; "\n"
                  &lt;&lt; "  Operations/sec: " &lt;&lt; std::fixed &lt;&lt; std::setprecision(2)
                  &lt;&lt; metrics.operations_per_second() &lt;&lt; "\n"
                  &lt;&lt; "  Current size: " &lt;&lt; store_.size() &lt;&lt; "\n";
    }
    
public:
    KVStoreCLI(size_t capacity, const std::string& snapshot_file = "kvstore.snap")
        : store_(capacity, snapshot_file), running_(true) {}
    
    void run() {
        std::cout &lt;&lt; "KVStore CLI - High Performance In-Memory Key-Value Store\n";
        std::cout &lt;&lt; "Type 'HELP' for available commands.\n\n";
        
        std::string line;
        while (running_ && std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            auto tokens = split(line, ' ');
            if (tokens.empty()) continue;
            
            std::string command = tokens[0];
            std::transform(command.begin(), command.end(), command.begin(), ::toupper);
            
            try {
                if (command == "GET" && tokens.size() == 2) {
                    std::string value;
                    if (store_.get(tokens[1], value)) {
                        std::cout &lt;&lt; "\"" &lt;&lt; value &lt;&lt; "\"\n";
                    } else {
                        std::cout &lt;&lt; "(nil)\n";
                    }
                }
                else if (command == "PUT" && tokens.size() >= 3) {
                    // Join all tokens after the key as the value
                    std::string value = tokens[2];
                    for (size_t i = 3; i &lt; tokens.size(); ++i) {
                        value += " " + tokens[i];
                    }
                    store_.put(tokens[1], value);
                    std::cout &lt;&lt; "OK\n";
                }
                else if (command == "DEL" && tokens.size() == 2) {
                    if (store_.remove(tokens[1])) {
                        std::cout &lt;&lt; "1\n";
                    } else {
                        std::cout &lt;&lt; "0\n";
                    }
                }
                else if (command == "CLEAR") {
                    store_.clear();
                    std::cout &lt;&lt; "OK\n";
                }
                else if (command == "SIZE") {
                    std::cout &lt;&lt; store_.size() &lt;&lt; "\n";
                }
                else if (command == "STATS") {
                    print_stats();
                }
                else if (command == "SAVE") {
                    store_.save_snapshot();
                    std::cout &lt;&lt; "Snapshot saved\n";
                }
                else if (command == "LOAD") {
                    if (store_.load_snapshot()) {
                        std::cout &lt;&lt; "Snapshot loaded\n";
                    } else {
                        std::cout &lt;&lt; "Failed to load snapshot\n";
                    }
                }
                else if (command == "HELP") {
                    print_help();
                }
                else if (command == "QUIT" || command == "EXIT") {
                    running_ = false;
                    std::cout &lt;&lt; "Goodbye!\n";
                }
                else {
                    std::cout &lt;&lt; "Unknown command. Type 'HELP' for available commands.\n";
                }
            }
            catch (const std::exception& e) {
                std::cout &lt;&lt; "Error: " &lt;&lt; e.what() &lt;&lt; "\n";
            }
            
            if (running_) {
                std::cout &lt;&lt; "kvstore> ";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    size_t capacity = 1000;  // Default capacity
    std::string snapshot_file = "kvstore.snap";
    
    // Parse command line arguments
    for (int i = 1; i &lt; argc; i++) {
        std::string arg = argv[i];
        if (arg == "--capacity" && i + 1 &lt; argc) {
            capacity = std::stoul(argv[++i]);
        } else if (arg == "--snapshot" && i + 1 &lt; argc) {
            snapshot_file = argv[++i];
        } else if (arg == "--help") {
            std::cout &lt;&lt; "Usage: " &lt;&lt; argv[0] &lt;&lt; " [options]\n"
                      &lt;&lt; "Options:\n"
                      &lt;&lt; "  --capacity <size>   Set cache capacity (default: 1000)\n"
                      &lt;&lt; "  --snapshot <file>   Set snapshot file (default: kvstore.snap)\n"
                      &lt;&lt; "  --help              Show this help\n";
            return 0;
        }
    }
    
    try {
        KVStoreCLI cli(capacity, snapshot_file);
        cli.run();
    }
    catch (const std::exception& e) {
        std::cerr &lt;&lt; "Fatal error: " &lt;&lt; e.what() &lt;&lt; std::endl;
        return 1;
    }
    
    return 0;
}
