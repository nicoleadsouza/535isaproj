#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>

using namespace std;

constexpr int CACHE_LINES = 16;
constexpr int WORDS_PER_LINE = 4;
constexpr int RAM_SIZE = 32768;
constexpr int MEMORY_DELAY = 3;

struct CacheLine {
    bool valid = false;
    bool dirty = false;
    int tag = -1;
    vector<int> data = vector<int>(WORDS_PER_LINE, 0);
};

class MemorySystem {
private:
    vector<int> ram;
    vector<CacheLine> cache;
    int cycle_count = 0;
    int memory_access_stage = -1;

public:
    MemorySystem() : ram(RAM_SIZE, 0), cache(CACHE_LINES) {}

    void write(int address, int value, int stage) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            cache[line_index].data[offset] = value;
            cache[line_index].dirty = true;
            cout << "done" << endl;
        } else {
            if (cycle_count == 0) {
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                cout << "wait" << endl;
            } else {
                if (memory_access_stage == stage) {
                    cycle_count--;
                    if (cycle_count == 0) {
                        ram[address] = value;
                        cout << "done" << endl;
                    } else {
                        cout << "wait" << endl;
                    }
                } else {
                    cout << "wait" << endl;
                }
            }
        }
    }

    void read(int address, int stage) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            cout << "done " << cache[line_index].data[offset] << endl;
        } else {
            if (cycle_count == 0) {
                cycle_count = MEMORY_DELAY;
                memory_access_stage = stage;
                cout << "wait" << endl;
            } else {
                if (memory_access_stage == stage) {
                    cycle_count--;
                    if (cycle_count == 0) {
                        cache[line_index].valid = true;
                        cache[line_index].tag = tag;
                        cache[line_index].dirty = false;
                        for (int i = 0; i < WORDS_PER_LINE; i++) {
                            cache[line_index].data[i] = ram[(address / WORDS_PER_LINE) * WORDS_PER_LINE + i];
                        }
                        cout << "done " << cache[line_index].data[offset] << endl;
                    } else {
                        cout << "wait" << endl;
                    }
                } else {
                    cout << "wait" << endl;
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

int main() {
    MemorySystem memSys;
    string command;
    int address, value, level, line, stage;

    while (cin >> command) {
        if (command == "W") {
            cin >> value >> address >> stage;
            memSys.write(address, value, stage);
        } else if (command == "R") {
            cin >> address >> stage;
            memSys.read(address, stage);
        } else if (command == "V") {
            cin >> level >> line;
            memSys.view(level, line);
        } else {
            cout << "Invalid command" << endl;
        }
    }
    return 0;
}
