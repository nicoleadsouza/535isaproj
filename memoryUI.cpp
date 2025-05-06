#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace std;

constexpr int CACHE_LINES = 16;
constexpr int WORDS_PER_LINE = 4;
constexpr int RAM_SIZE = 32768;
constexpr int MEMORY_DELAY = 3;
constexpr int CACHE_DELAY = 1;

constexpr int STATUS_WAIT = 0;
constexpr int STATUS_DONE = 1;

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    int tag = -1;
    vector<int> data = vector<int>(WORDS_PER_LINE, 0);
};

struct MemoryResult {
    int status;
    int value;
};

class MemorySystem {
private:
    vector<int> ram;
    vector<CacheLine> cache;
    int cycle_count = 0;
    int memory_access_stage = -1;
    bool useCache;
    bool accessing_cache = false;
    bool accessing_ram = false;

public:
    MemorySystem(bool cache) : ram(RAM_SIZE, 0), cache(CACHE_LINES), useCache(cache) {}
    MemorySystem() : ram(RAM_SIZE, 0), cache(CACHE_LINES) {}

    MemoryResult write(int address, int value, int stage) {
        if ((accessing_cache || accessing_ram) && memory_access_stage != stage) return {STATUS_WAIT, 0}; // memory occupied
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (useCache && cache[line_index].valid && cache[line_index].tag == tag) { // in cache
            if (!accessing_cache) {
                accessing_cache = true;
                accessing_ram = false;
                cycle_count = CACHE_DELAY;
                memory_access_stage = stage;
                return {STATUS_WAIT, 0};
            } else {
                cycle_count--;
                if (cycle_count == 0) {
                    accessing_cache = false;
                    cache[line_index].data[offset] = value;
                    cache[line_index].dirty = true;
                    return {STATUS_DONE, 0};
                }
                return {STATUS_WAIT, 0};
            }
        } else { // not in cache
            if (!accessing_ram) {
                accessing_ram = true;
                accessing_cache = false;
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                return {STATUS_WAIT, 0};
            } else {
                cycle_count--;
                if (cycle_count == 0) {
                    accessing_ram = false;
                    ram[address] = value;
                    return {STATUS_DONE, 0};
                }
                return {STATUS_WAIT, 0};
            }
        }
    }

    MemoryResult read(int address, int stage) {
        if ((accessing_cache || accessing_ram) && memory_access_stage != stage) return {STATUS_WAIT, 0}; // memory occupied
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (useCache && cache[line_index].valid && cache[line_index].tag == tag) {
            // cout << "Cache hit!" << endl;
            if (!accessing_cache) {
                accessing_cache = true;
                accessing_ram = false;
                cycle_count = CACHE_DELAY;
                memory_access_stage = stage;
                return {STATUS_WAIT, 0};
            } else {
                cycle_count--;
                if (cycle_count == 0) {
                    accessing_cache = false;
                    return {STATUS_DONE, cache[line_index].data[offset]};
                }
                return {STATUS_WAIT, 0};
            }
        } else {
            if (!accessing_ram) {
                accessing_ram = true;
                accessing_cache = false;
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                return {STATUS_WAIT, 0};
            } else {
                cycle_count--;
                if (cycle_count == 0) {
                    accessing_ram = false;
                    if (useCache) {
                        if (cache[line_index].dirty) { 
                            int oldaddr = (cache[line_index].tag * (CACHE_LINES * WORDS_PER_LINE)) + (line_index * WORDS_PER_LINE);
                            for (int i = 0; i < WORDS_PER_LINE; i++) {
                                ram[((oldaddr / WORDS_PER_LINE) * WORDS_PER_LINE) + i] = cache[line_index].data[i];
                            }
                        }
                        cache[line_index].valid = true;
                        cache[line_index].tag = tag;
                        cache[line_index].dirty = false;
                        for (int i = 0; i < WORDS_PER_LINE; i++) {
                            cache[line_index].data[i] = ram[((address / WORDS_PER_LINE) * WORDS_PER_LINE) + i];
                        }
                        return {STATUS_DONE, cache[line_index].data[offset]};
                    } else {
                        return {STATUS_DONE, ram[address]};
                    }
                }
                return {STATUS_WAIT, 0};
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

    // for testing/demoing, please leave these here until we begin to start on full demo

    void forceWrite(int address, int value) {
        if (address >= 0 && address < RAM_SIZE) {
            ram[address] = value;
        }
    }

    int forceRead(int address) {
        if (address >= 0 && address < RAM_SIZE) {
            return ram[address];
        }
        return 0;
    }
};
