#pragma once
#include <iostream>
#include <vector>

using namespace std;

const int RAM_SIZE = 1024;
const int CACHE_LINES = 4;
const int WORDS_PER_LINE = 4;

const int STATUS_WAIT = 0;
const int STATUS_DONE = 1;

struct MemoryResult {
    int status;
    int value;
};

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    int tag = -1;
    vector<int> data = vector<int>(WORDS_PER_LINE, 0);
};

class MemorySystem {
private:
    vector<int> ram = vector<int>(RAM_SIZE, 0);
    vector<CacheLine> cache = vector<CacheLine>(CACHE_LINES);
    bool useCache = true;
    int hits = 0;
    int misses = 0;

public:
    MemorySystem(bool enableCache = true) {
        useCache = enableCache;
    }

    MemoryResult read(int address, int stage) {
        if (address < 0 || address >= RAM_SIZE) return {STATUS_DONE, 0};

        if (!useCache) {
            return {STATUS_DONE, ram[address]};
        }

        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        CacheLine& line = cache[line_index];

        if (line.valid && line.tag == tag) {
            hits++;
            return {STATUS_DONE, line.data[offset]};
        } else {
            misses++;
            if (line.valid && line.dirty) {
                int oldaddr = (line.tag * CACHE_LINES * WORDS_PER_LINE) + (line_index * WORDS_PER_LINE);
                for (int i = 0; i < WORDS_PER_LINE; i++) {
                    ram[oldaddr + i] = line.data[i];
                }
            }

            line.valid = true;
            line.dirty = false;
            line.tag = tag;

            int base_addr = (address / WORDS_PER_LINE) * WORDS_PER_LINE;
            for (int i = 0; i < WORDS_PER_LINE; i++) {
                line.data[i] = ram[base_addr + i];
            }

            return {STATUS_DONE, line.data[offset]};
        }
    }

    MemoryResult write(int address, int value, int stage) {
        if (address < 0 || address >= RAM_SIZE) return {STATUS_DONE, 0};

        if (!useCache) {
            ram[address] = value;
            return {STATUS_DONE, 0};
        }

        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        CacheLine& line = cache[line_index];

        if (line.valid && line.tag == tag) {
            hits++;
            line.data[offset] = value;
            line.dirty = true;
            return {STATUS_DONE, 0};
        } else {
            misses++;
            if (line.valid && line.dirty) {
                int oldaddr = (line.tag * CACHE_LINES * WORDS_PER_LINE) + (line_index * WORDS_PER_LINE);
                for (int i = 0; i < WORDS_PER_LINE; i++) {
                    ram[oldaddr + i] = line.data[i];
                }
            }

            line.valid = true;
            line.dirty = true;
            line.tag = tag;

            int base_addr = (address / WORDS_PER_LINE) * WORDS_PER_LINE;
            for (int i = 0; i < WORDS_PER_LINE; i++) {
                line.data[i] = ram[base_addr + i];
            }

            line.data[offset] = value;

            return {STATUS_DONE, 0};
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

    int getHits() const { return hits; }
    int getMisses() const { return misses; }
};
