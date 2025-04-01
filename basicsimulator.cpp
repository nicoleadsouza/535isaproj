#include <iostream>
#include <vector>
#include <unordered_map>
#include <iomanip>
// #include <QtWidgets/QApplication>
// #include <QtWidgets/QMainWindow>
// #include <QtWidgets/QTableWidget>
// #include <QtWidgets/QPushButton> // for Qt UI
#include "memoryUI.cpp"

//g++ basicsimulator.cpp -std=c++11 -stdlib=libc++

using namespace std;

// constexpr int CACHE_LINES = 16;
// constexpr int WORDS_PER_LINE = 4;
// constexpr int RAM_SIZE = 32768;
// constexpr int MEMORY_DELAY = 3;
constexpr int NUM_REGISTERS = 16;

constexpr int STAGE_FETCH = 0;
constexpr int STAGE_MEMORY = 1;

constexpr int TYPE_ALU = 0;
constexpr int TYPE_CONTROL = 1;

// struct CacheLine {
//     bool valid = false;
//     bool dirty = false;
//     int tag = -1;
//     vector<int> data = vector<int>(WORDS_PER_LINE, 0);
// };

// class MemorySystem {
// private:
//     vector<int> ram;
//     vector<CacheLine> cache;
//     int cycle_count = 0;
//     int memory_access_stage = -1;

// public:
//     MemorySystem() : ram(RAM_SIZE, 0), cache(CACHE_LINES) {}

//     void write(int address, int value) {
//         int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
//         int tag = address / (CACHE_LINES * WORDS_PER_LINE);
//         int offset = address % WORDS_PER_LINE;

//         if (cache[line_index].valid && cache[line_index].tag == tag) {
//             cache[line_index].data[offset] = value;
//             cache[line_index].dirty = true;
//         } else {
//             ram[address] = value;
//         }
//     }

//     int read(int address) {
//         int line_index = (address / WORDS_PER_LINE) % CACHE_LINES;
//         int tag = address / (CACHE_LINES * WORDS_PER_LINE);
//         int offset = address % WORDS_PER_LINE;

//         if (cache[line_index].valid && cache[line_index].tag == tag) {
//             return cache[line_index].data[offset];
//         } else {
//             return ram[address];
//         }
//     }
// };

struct Instruction {
    int type;
    int opcode;
    int r0, r1, r2;
    int cond;
    int immediate;
    int op1, op2;
    int result;
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
            // case 0: // load
            //     registers[instr.r0] = memory.read(registers[instr.r1] + instr.immediate);
            //     break;
            // case 1: // store
            //     memory.write(registers[instr.r1] + instr.immediate, registers[instr.r0]);
            //     break;
            // case 5: // add
            //     registers[instr.r0] = registers[instr.r1] + registers[instr.r2];
            //     break;
            // case 7: // sub
            //     registers[instr.r0] = registers[instr.r1] - registers[instr.r2];
            //     break;
            // case 20: // branch
            //     if (registers[instr.r0] == registers[instr.r1])
            //         program_counter += instr.immediate;
            //     break;
        }
    }

    int fetch(long pc) {
        MemoryResult inst = memory.read(pc, 0);
        if (inst.status == STATUS_DONE) {
            return inst.value;
        }
        // TODO: handle stall for cache miss...
    }

    Instruction decode(unsigned int inst) {
        // TODO: may need to wait for operands to be available...
        int opcode = (inst & 0xF8000000) >> 27; // highest 5 bits are the opcode
        cout << "Opcode: " << opcode << endl;
        if (opcode == 20) { // branch, format C: 5 bits opcode, 4 bits r0, 4 bits r1, 2 bits cond, 17 bits imm
            int r0 = (inst & 0x07800000) >> 23;
            cout << "R0: " << r0 << endl;
            int r1 = (inst & 0x00780000) >> 19;
            cout << "R1: " << r1 << endl;
            int cond = (inst & 0x00060000) >> 17;
            cout << "condition: " << cond << endl;
            int imm = inst & 0x0001FFFF;
            cout << "immediate: " << imm << endl;
            return {type: TYPE_CONTROL, opcode: opcode, r0: r0, r1: r1, cond: cond, immediate: imm};
        } else { // instruction format A: 5 bits opcode, 4 bits r0, 4 bits r1, 4 bits r2, 15 bits imm
            int r0 = (inst & 0x07800000) >> 23;
            cout << "R0: " << r0 << endl;
            int r1 = (inst & 0x00780000) >> 19;
            cout << "R1: " << r1 << endl;
            int r2 = (inst & 0x00078000) >> 15;
            cout << "R2: " << r2 << endl;
            int imm = inst & 0x00007FFF;
            cout << "immediate: " << imm << endl;
            return {type: TYPE_ALU, opcode: opcode, r0: r0, r1: r1, r2: r2, immediate: imm};
        }
    }

    Instruction execute(Instruction inst) {
        // TODO: execute
    }

    Instruction memory(Instruction inst) {
        // TODO: memory
    }

    void writeback(Instruction inst) {
        // TODO: writeback
    }
};

int main(int argc, char *argv[]) {
    // QApplication app(argc, argv);
    // QMainWindow window;
    // window.setWindowTitle("CacheFlow Simulator");
    // window.show();
    // return app.exec();
    Simulator sim;
    int opcode = 0;
    int r1 = 15;
    int r2 = 7;
    int r3 = 3;
    int imm = 1456;
    int inst = (opcode << 27) | (r1 << 23) | (r2 << 19) | (r3 << 15) | imm;
    printf("%x \n", inst);
    string binary_inst = bitset<32>(inst).to_string();
    cout << binary_inst << endl;
    Instruction decoded = sim.decode(inst);
}
