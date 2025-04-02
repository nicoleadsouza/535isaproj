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
constexpr int STAGE_DECODE = 1;
constexpr int STAGE_EXECUTE = 2;
constexpr int STAGE_MEMORY = 3;
constexpr int STAGE_WRITEBACK = 4;

constexpr int TYPE_ALU = 0;
constexpr int TYPE_CONTROL = 1;
constexpr int TYPE_MEMORY = 2;

constexpr int FLAG_HALT = 1;
constexpr int FLAG_RUNNING = 0;

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
    int addr;
    unsigned int binary;
    int type;
    int opcode;
    int r0, r1, r2;
    int cond;
    int immediate;
    int op1, op2;
    int result;
    int writeback_val;
    bool has_writeback;
    bool stall;
    bool is_empty = true;
};

class Simulator {
private:
    vector<int> registers;
    int program_counter;
    MemorySystem memory_system;
    vector<Instruction> instructions;
    int cycle_count = 0;
    bool pipeline_halted = false;

    vector<Instruction> pipeline = vector<Instruction>(5);

    bool evaluateCond(int cond, int op1, int op2) {
        switch (cond) {
            case 0: // equal
                return op1 == op2;
            case 1: // less than
                return op1 < op2;
            case 2: // greater than or equal
                return op1 >= op2;
            case 3: // not equal
                return op1 != op2;
        }
    }

    string getStageDisplay(Instruction inst, int stage) {
        if (inst.is_empty) {
            return "empty";
        } else if (inst.stall) {
            return "stall";
        } else if (stage == STAGE_FETCH) {
            return to_string(inst.binary);
        } else {
            string operation;
            switch (inst.opcode) {
                case 0:
                    operation = "LOAD";
                    break;
                case 1:
                    operation = "STR";
                    break;
                case 5:
                    operation = "ADD";
                    break;
                case 7:
                    operation = "SUB";
                    break;
                case 20:
                    operation = "BRN";
                    break;
                default:
                    operation = "NOP";
            }
            return operation;
        }
    }

public:
    Simulator() : registers(NUM_REGISTERS, 0), program_counter(0) {}

    void loadProgram(vector<Instruction> instrs) {
        instructions = instrs;
    }

    int step() {
        // writeback
        if (!pipeline[STAGE_WRITEBACK].is_empty) {
            int res = writeback(pipeline[STAGE_WRITEBACK]);

            pipeline[STAGE_WRITEBACK].is_empty = true;

            if (res == FLAG_HALT) {
                return FLAG_HALT;
            }
        }

        // memory
        if (!pipeline[STAGE_MEMORY].is_empty) {
            Instruction res = memory(pipeline[STAGE_MEMORY]);

            pipeline[STAGE_WRITEBACK] = res;

            if (!res.stall) pipeline[STAGE_MEMORY].is_empty = true;
        }

        // execute
        if (!pipeline[STAGE_EXECUTE].is_empty) {
            Instruction res = execute(pipeline[STAGE_EXECUTE]);

            pipeline[STAGE_MEMORY] = res;

            if (!res.stall) pipeline[STAGE_EXECUTE].is_empty = true;
        }

        // decode
        if (!pipeline[STAGE_DECODE].is_empty) {
            Instruction res = decode(pipeline[STAGE_DECODE]);

            pipeline[STAGE_EXECUTE] = res;

            if (!res.stall) pipeline[STAGE_DECODE].is_empty = true;
        }

        // fetch
        if (!pipeline[STAGE_FETCH].is_empty) {
            Instruction res = fetch(pipeline[STAGE_FETCH]);

            pipeline[STAGE_DECODE] = res;

            if (!res.stall) pipeline[STAGE_FETCH].is_empty = true;
        }

        if (pipeline[STAGE_FETCH].is_empty && !pipeline_halted) {
            pipeline[STAGE_FETCH] = {addr: program_counter};
        }

        cycle_count++;

        return FLAG_RUNNING;
    }

    Instruction fetch(Instruction inst) {
        if (inst.is_empty) {
            return inst;
        }

        MemoryResult res = memory_system.read(inst.addr, STAGE_FETCH);
        if (res.status == STATUS_DONE) {
            return {binary: (unsigned int) res.value, is_empty: false};
        } else { // stall
            return {stall: true};
        }
    }

    Instruction decode(Instruction inst) {
        // TODO: deal with operands and dependencies
        if (inst.stall || inst.is_empty) { // pass along a stall
            return inst;
        }

        if (inst.binary == -1) { // special invalid operand indicates halt
            pipeline_halted = true;
            return {is_empty: true};
        }

        Instruction res;
        int opcode = (inst.binary & 0xF8000000) >> 27; // highest 5 bits are the opcode

        // cout << "Opcode: " << opcode << endl;

        if (opcode == 20) { // branch
            //format C: 5 bits opcode, 4 bits r0, 4 bits r1, 2 bits cond, 17 bits imm

            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int cond = (inst.binary & 0x00060000) >> 17;
            int imm = inst.binary & 0x0001FFFF;

            // cout << "R0: " << r0 << endl;
            // cout << "R1: " << r1 << endl;
            // cout << "condition: " << cond << endl;
            // cout << "immediate: " << imm << endl;

            // TODO: use old inst instead
            res = {type: TYPE_CONTROL, opcode: opcode, r0: r0, r1: r1, cond: cond, immediate: imm, has_writeback: false};
        } else {
            // format A: 5 bits opcode, 4 bits r0, 4 bits r1, 4 bits r2, 15 bits imm
            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int r2 = (inst.binary & 0x00078000) >> 15;
            int imm = inst.binary & 0x00007FFF;

            // cout << "R0: " << r0 << endl;
            // cout << "R1: " << r1 << endl;
            // cout << "R2: " << r2 << endl;
            // cout << "immediate: " << imm << endl;

            int type = (opcode == 0 || opcode == 1) ? TYPE_MEMORY : TYPE_ALU;
            bool has_writeback = opcode == 1 ? false : true;

            res = {type: type, opcode: opcode, r0: r0, r1: r1, r2: r2, immediate: imm, has_writeback: has_writeback};
        }

        return res;
    }

    Instruction execute(Instruction inst) {
        if (inst.stall || inst.is_empty) { // pass along a stall
            return inst;
        }

        int res;
        switch (inst.opcode) {
            case 0: // load
                // same as store, so fall through
            case 1: // store
                res = inst.op1 + inst.op1 + inst.immediate;
                break;
            case 5: // add
                res = inst.op1 + inst.op2;
                break;
            case 7: // sub
                res = inst.op1 - inst.op2;
                break;
            case 20: // branch
                if (evaluateCond(inst.cond, inst.op1, inst.op2)) {
                    program_counter += inst.immediate;
                    // TODO: squash everything in the pipe
                }
                break;
        }
        
        inst.result = res;
        return inst;
    }

    Instruction memory(Instruction inst) {
        if (inst.stall || inst.is_empty) { // pass along a stall
            return inst;
        }

        if (inst.type != TYPE_MEMORY) {
            return inst;
        }
        if (inst.opcode == 0) { // load
            MemoryResult res = memory_system.read(inst.result, STAGE_MEMORY);
            if (res.status == STATUS_DONE) {
                inst.writeback_val = res.value;
                return inst;
            } // TODO: else stall...
        } else { // == 1, store
            MemoryResult res = memory_system.write(inst.result, inst.op1, STAGE_MEMORY);
            // TODO: if status not done, stall...
            return inst;
        }
    }

    int writeback(Instruction inst) {
        if (inst.is_empty) return FLAG_HALT;
        if (inst.has_writeback) {
            registers[inst.r0] = inst.writeback_val;
        }
        return FLAG_RUNNING;
    }

    void displayPipeline() {
        cout << " | Fetch | Decode | Execute | Memory | Writeback | " << endl;
        cout << " | ";
        for (int i = 0; i < 5; i++) {
            cout << getStageDisplay(pipeline[i], i) << " | ";
        }
    }
};

int main(int argc, char *argv[]) {
    // QApplication app(argc, argv);
    // QMainWindow window;
    // window.setWindowTitle("CacheFlow Simulator");
    // window.show();
    // return app.exec();

}
