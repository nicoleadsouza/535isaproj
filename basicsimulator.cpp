#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton> // for Qt UI

using namespace std;

constexpr int CACHE_LINES = 16;
constexpr int WORDS_PER_LINE = 4;
constexpr int RAM_SIZE = 32768;
constexpr int MEMORY_DELAY = 3;
constexpr int NUM_REGISTERS = 16;

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

    void write(int address, int value) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            cache[line_index].data[offset] = value;
            cache[line_index].dirty = true;
        } else {
            ram[address] = value;
        }
    }

    int read(int address) {
        int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
        int tag = address / (CACHE_LINES * WORDS_PER_LINE);
        int offset = address % WORDS_PER_LINE;

        if (cache[line_index].valid && cache[line_index].tag == tag) {
            return cache[line_index].data[offset];
        } else {
            return ram[address];
        }
    }
};

struct Instruction {
    int opcode;
    int r0, r1, r2;
    int immediate;
};

class Simulator {
private:
    vector<int> registers;
    int program_counter;
    MemorySystem memory;
    vector<Instruction> instructions;
    int cycle_count = 0;

public:
    Simulator() : registers(NUM_REGISTERS, 0), program_counter(0) {}

    void loadProgram(vector<Instruction> instrs) {
        instructions = instrs;
    }

    void step() {
        if (program_counter >= instructions.size()) return;
        Instruction instr = instructions[program_counter++];
        cycle_count++;
        
        switch (instr.opcode) {
            case 0: // load
                registers[instr.r0] = memory.read(registers[instr.r1] + instr.immediate);
                break;
            case 1: // store
                memory.write(registers[instr.r1] + instr.immediate, registers[instr.r0]);
                break;
            case 5: // add
                registers[instr.r0] = registers[instr.r1] + registers[instr.r2];
                break;
            case 7: // sub
                registers[instr.r0] = registers[instr.r1] - registers[instr.r2];
                break;
            case 20: // branch
                if (registers[instr.r0] == registers[instr.r1])
                    program_counter += instr.immediate;
                break;
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle("CacheFlow Simulator");
    window.show();
    return app.exec();
}
