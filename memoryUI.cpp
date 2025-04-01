#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace std;

constexpr int CACHE_LINES = 16;
constexpr int WORDS_PER_LINE = 4;
constexpr int RAM_SIZE = 32768;
constexpr int MEMORY_DELAY = 3;

constexpr int STATUS_WAIT = 0;
constexpr int STATUS_DONE = 1;

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    int tag = -1;
    vector<int> data = vector<int>(WORDS_PER_LINE, 0);
};

struct MemoryResult {
    int status; // 0 = wait, 1 = done
    int value; // only a reasonable value for reads
};

class MemorySystem {
private:
    vector<int> ram;
    vector<CacheLine> cache;
    int cycle_count = 0;
    int memory_access_stage = -1;

public:
    MemorySystem() : ram(RAM_SIZE, 0), cache(CACHE_LINES) {}

    MemoryResult write(int address, int value, int stage) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            cache[line_index].data[offset] = value;
            cache[line_index].dirty = true;
            return {status: STATUS_DONE, value: 0};
        } else {
            if (cycle_count == 0) {
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                return {status: STATUS_WAIT, value: 0};
            } else {
                if (memory_access_stage == stage) {
                    cycle_count--;
                    if (cycle_count == 0) {
                        ram[address] = value;
                        return {status: STATUS_DONE, value: 0};
                    } else {
                        return {status: STATUS_WAIT, value: 0};
                    }
                } else {
                    return {status: STATUS_WAIT, value: 0};
                }
            }
        }
    }

    MemoryResult read(int address, int stage) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            return {status: STATUS_DONE, value: cache[line_index].data[offset]};
        } else {
            if (cycle_count == 0) {
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                return {status: STATUS_WAIT, value: 0};
            } else {
                if (memory_access_stage == stage) {
                    cycle_count--;
                    if (cycle_count == 0) {
                        if (cache[line_index].dirty) {
                            // line_index = address % cache size
                            // tag = address / cache size
                            // so address = (tag * cache size) + line_index
                            int oldaddr = (cache[line_index].tag * (CACHE_LINES * WORDS_PER_LINE)) + line_index;
                            for (int i = 0; i < WORDS_PER_LINE; i++) {
                                ram[(oldaddr / WORDS_PER_LINE) * WORDS_PER_LINE + i] = cache[line_index].data[i];
                            }
                        }
                        cache[line_index].valid = true;
                        cache[line_index].tag = tag;
                        cache[line_index].dirty = false;
                        for (int i = 0; i < WORDS_PER_LINE; i++) {
                            cache[line_index].data[i] = ram[(address / WORDS_PER_LINE) * WORDS_PER_LINE + i];
                        }
                        return {status: STATUS_DONE, value: cache[line_index].data[offset]};
                    } else {
                        return {status: STATUS_WAIT, value: 0};
                    }
                } else {
                    return {status: STATUS_WAIT, value: 0};
                }
            }
        }
    }

    void view(int level, int line) {
        if (level == 1 && line < CACHE_LINES) {
            cout << "Cache Line " << line << " [Valid: " << cache[line].valid 
                 << ", Tag: " << cache[line].tag << ", Dirty: " << cache[line].dirty << "] - ";
            for (int i : cache[line].data) cout << i << " ";
            cout << endl;
        } else if (level == 0 && line < RAM_SIZE / WORDS_PER_LINE) {
            cout << "RAM Line " << line << " - ";
            for (int i = 0; i < WORDS_PER_LINE; i++)
                cout << ram[line * WORDS_PER_LINE + i] << " ";
            cout << endl;
        } else {
            cout << "Invalid view command" << endl;
        }
    }
};

// int main() {
//     MemorySystem memSys;
//     string command;
//     int address, value, level, line, stage;

//     while (cin >> command) {
//         if (command == "W") {
//             cin >> value >> address >> stage;
//             memSys.write(address, value, stage);
//         } else if (command == "R") {
//             cin >> address >> stage;
//             memSys.read(address, stage);
//         } else if (command == "V") {
//             cin >> level >> line;
//             memSys.view(level, line);
//         } else {
//             cout << "Invalid command" << endl;
//         }
//     }
//     return 0;
// }
