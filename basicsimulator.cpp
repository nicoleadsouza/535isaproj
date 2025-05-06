#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iomanip>
#include "memoryUI.cpp" // includes memory system

using namespace std;

// simulator constants
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

// instruction structure
struct Instruction {
    int addr = -1;
    unsigned int binary = -1;
    int type = -1;
    int opcode = -1;
    int r0 = -1, r1 = -1, r2 = -1;
    int cond = -1;
    int immediate = -1;
    int op1 = -1, op2 = -1, op3 = -1;
    int result = -1;
    int writeback_val = -1;
    int target = -1;
    bool has_writeback = false;
    bool hazard = false;
    bool stall = false;
    bool is_empty = true;
};

class Simulator {
private:
    vector<int> registers;
    int program_counter;
    MemorySystem memory_system;
    int cycle_count = 0;
    bool pipeline_halted = false;
    vector<Instruction> pipeline = vector<Instruction>(5);
    bool use_pipeline;
    bool keep_fetching = true;

    char getInstType(int opcode) {
        switch (opcode) { // uses fallthrough intentionally
            case 0:
            case 1:
            case 2:
            case 5:
            case 7:
            case 9:
            case 11:
            case 13:
            case 16:
            case 17:
            case 18:
            case 19: return 'A';
            case 6:
            case 8:
            case 10:
            case 12:
            case 14: return 'B';
            case 15:
            case 20: return 'C';
            case 3:
            case 4:
            case 21:
            case 22: return 'D';
            default: return 'X'; // unrecognized instruction, treat as NOP
        }
    }

    bool evaluateCond(int cond, int op1, int op2) {
        switch (cond) {
            case 0: return op1 == op2;
            case 1: return op1 < op2;
            case 2: return op1 >= op2;
            case 3: return op1 != op2;
            default: return false;
        }
    }

    string getStageDisplay(Instruction inst, int stage) const {
        if (inst.is_empty) return "empty";
        if (inst.stall) return "stall";
        if (inst.hazard) return "hazard";
        if (inst.opcode == -1) return to_string(inst.addr);
        return getOperationName(inst.opcode);
    }

    string getOperationName(int opcode) const {
        switch (opcode) {
            case 0: return "LOAD";
            case 1: return "STR";
            case 3: return "LOADI";
            case 5: return "ADD";
            case 7: return "SUB";
            case 9: return "MUL";
            case 20: return "BRN";
            case 21: return "JUMP";
            default: return "NOP";
        }
    }

public:
    //Simulator() : registers(NUM_REGISTERS, 0), program_counter(0) {}
    // optionally can specifify whether to use pipeline and cache
    Simulator(bool pipe = true, bool cache = true) : registers(NUM_REGISTERS, 0), program_counter(0), use_pipeline(pipe), memory_system(cache) {}

    // --- ADDED public getter methods for Qt UI ---
    int getCycleCount() const { return cycle_count; }
    int getProgramCounter() const { return program_counter; }
    int viewRegister(int reg) const { return registers[reg]; }
    string getStageDisplayText(int stage) const { return getStageDisplay(pipeline[stage], stage); }
    // ------------------------------------------------

    void loadProgramFromFile(const string& filename) {
        ifstream infile(filename);
        unsigned int instr;
        int addr = 0;
        while (infile >> instr) {
            memory_system.forceWrite(addr, instr);
            addr++;
        }

        // hard coded this here for testing/demoing purposes
        // memory_system.forceWrite(100, 42);
        // registers[0] = 100;

        program_counter = 0;
        pipeline = vector<Instruction>(5);
    }

    int step() {
        if (!pipeline[STAGE_WRITEBACK].is_empty) {
            if (writeback(pipeline[STAGE_WRITEBACK]) == FLAG_HALT)
                return FLAG_HALT;
            pipeline[STAGE_WRITEBACK].is_empty = true;
            keep_fetching = true;
        }

        if (!pipeline[STAGE_MEMORY].is_empty) {
            pipeline[STAGE_MEMORY].stall = false;
            if (pipeline[STAGE_WRITEBACK].is_empty) {
                Instruction res = memory(pipeline[STAGE_MEMORY]);
                if (!res.hazard)  {
                    pipeline[STAGE_WRITEBACK] = res;
                    pipeline[STAGE_MEMORY].is_empty = true;
                } else {
                    pipeline[STAGE_MEMORY].stall = true;
                }
            } else {
                pipeline[STAGE_MEMORY].stall = true;
            }
        }

        if (!pipeline[STAGE_EXECUTE].is_empty) {
            pipeline[STAGE_EXECUTE].stall = false;
            if (pipeline[STAGE_MEMORY].is_empty) {
                Instruction res = execute(pipeline[STAGE_EXECUTE]);
                if (!res.hazard) {
                    pipeline[STAGE_MEMORY] = res;
                    pipeline[STAGE_EXECUTE].is_empty = true;
                } else {
                    pipeline[STAGE_EXECUTE].stall = true;
                }
            } else {
                pipeline[STAGE_EXECUTE].stall = true;
            }
        }

        if (!pipeline[STAGE_DECODE].is_empty) {
            pipeline[STAGE_DECODE].stall = false;
            if (pipeline[STAGE_EXECUTE].is_empty) {
                Instruction res = decode(pipeline[STAGE_DECODE]);
                if (!res.hazard) {
                    pipeline[STAGE_EXECUTE] = res;
                    pipeline[STAGE_DECODE].is_empty = true;
                } else {
                    pipeline[STAGE_DECODE].stall = true;
                }
            } else {
                pipeline[STAGE_DECODE].stall = true;
            }
        }

        if (!pipeline[STAGE_FETCH].is_empty) { 
            pipeline[STAGE_FETCH].stall = false;
            if (pipeline[STAGE_DECODE].is_empty) {
                Instruction res = fetch(pipeline[STAGE_FETCH]);
                if (!res.hazard) {
                    pipeline[STAGE_DECODE] = res;
                    pipeline[STAGE_FETCH].is_empty = true;
                } else {
                    pipeline[STAGE_FETCH].stall = true;
                }
            } else {
                pipeline[STAGE_FETCH].stall = true;
            }
        }

        if (pipeline[STAGE_FETCH].is_empty && !pipeline_halted && keep_fetching) {
            Instruction inst;
            inst.addr = program_counter;
            inst.is_empty = false;
            pipeline[STAGE_FETCH] = inst;
            if (!use_pipeline) keep_fetching = false;
        }

        cycle_count++;
        return FLAG_RUNNING;
    }

    Instruction fetch(Instruction inst) {
        if (inst.is_empty) return inst;
    
        // halt fetch if PC goes past reasonable program memory range
        if (program_counter >= RAM_SIZE || program_counter < 0) {
            pipeline_halted = true;
            Instruction halt_inst;
            halt_inst.is_empty = true;
            return halt_inst;
        }
    
        MemoryResult res = memory_system.read(inst.addr, STAGE_FETCH);
        if (res.status == STATUS_DONE) {
            Instruction new_inst;
            new_inst.binary = res.value;
            new_inst.addr = inst.addr;
    
            if (res.value == -1) {
                // treat -1 (invalid instruction) as HALT signal
                pipeline_halted = true;
                new_inst.is_empty = true;
            } else {
                new_inst.is_empty = false;
                program_counter++;
            }
    
            return new_inst;
        } else {
            cout << "fetch for instruction " << inst.addr << " missed cache, waiting for RAM" << endl;
            inst.hazard = true;
            return inst;
        }
    }    

    Instruction decode(Instruction inst) {
        if (inst.is_empty) return inst;
        if (inst.binary == -1) {
            pipeline_halted = true;
            Instruction halt_inst;
            halt_inst.is_empty = true;
            return halt_inst;
        }

        Instruction res;
        res.addr = inst.addr;
        int opcode = (inst.binary & 0xF8000000) >> 27;
        char inst_type = getInstType(opcode);
        res.opcode = opcode;

        switch (inst_type) {
            case 'A':
                // either 0 (LOAD), 1 (STR), or ALU
                res.type = (opcode == 0 || opcode == 1) ? TYPE_MEMORY : TYPE_ALU;
                res.r0 = (inst.binary & 0x07800000) >> 23;
                res.r1 = (inst.binary & 0x00780000) >> 19;
                res.r2 = (inst.binary & 0x00078000) >> 15;
                res.op1 = res.r1;
                res.op2 = res.r2;
                if (res.opcode == 1) res.op3 = res.r0; // STR has a third operand
                res.target = res.r0;
                res.immediate = inst.binary & 0x00007FFF;
                if (res.immediate < 0) res.immediate = res.immediate | 0xFFFF8000; // sign extend if necessary
                res.has_writeback = opcode != 1; // STR has no writeback value
                break;
            case 'B':
                // all ALU operations
                res.r0 = (inst.binary & 0x07800000) >> 23;
                res.r1 = (inst.binary & 0x00780000) >> 19;
                res.immediate = inst.binary & 0x0007FFFF;
                if (res.immediate < 0) res.immediate = res.immediate | 0xFFF80000; // sign extend if necessary
                res.type = TYPE_ALU;
                res.op1 = res.r1;
                res.target = res.r0;
                res.has_writeback = true;
                break;
            case 'C':
                // either 20 (BRN) or 15 (SHF)
                res.r0 = (inst.binary & 0x07800000) >> 23;
                res.r1 = (inst.binary & 0x00780000) >> 19;
                res.cond = (inst.binary & 0x00060000) >> 17;
                res.immediate = inst.binary & 0x0001FFFF;
                if (res.immediate < 0) res.immediate = res.immediate | 0xFFF60000; // sign extend if necessary
                res.type = opcode == 20 ? TYPE_CONTROL : TYPE_ALU;
                res.op1 = res.r0;
                res.op2 = res.r1;
                res.has_writeback = opcode == 15; // SHF has a rightback value, but BRN does not
                break;
            case 'D':
                // either 3 (LOADI) or control
                // despite the name, LOADI is an ALU operation as it does not access memory, only registers
                res.r0 = (inst.binary & 0x07800000) >> 23;
                res.immediate = inst.binary & 0x007FFFFF;
                if (res.immediate < 0) res.immediate = res.immediate | 0xFF800000; // sign extend if necessary
                if (opcode == 3) {
                    res.target = res.r0;
                    res.op1 = res.immediate;
                    res.type = TYPE_ALU;
                    res.has_writeback = true;
                } else {
                    res.op1 = res.r0;
                    res.type = TYPE_CONTROL;
                }
                break;
        }

        // handle dependencies
        // search for instructions in pipe targeting the operands
        for (int i = STAGE_EXECUTE; i <= STAGE_WRITEBACK; i++) {
            if (!pipeline[i].is_empty && pipeline[i].has_writeback && (pipeline[i].target == res.op1 || pipeline[i].target == res.op2 || pipeline[i].target == res.op3)) {
                cout << "instruction " << res.addr << "(" << getOperationName(res.opcode) << ")";
                cout << " has dependency on instruction " << pipeline[i].addr <<"(" << getOperationName(pipeline[i].opcode) << ")" << endl;
                // dependency is in the pipe, so we need to stall
                res.hazard = true;
                res.is_empty = false;
                return res;
            }
        }
        // no dependencies in pipe, fetch operands
        if (opcode != 3) res.op1 = registers[res.op1]; // LOADI has only an immediate operand
        if (inst_type == 'A' || inst_type == 'C') res.op2 = registers[res.op2];
        if (res.op3 != -1) res.op3 = registers[res.op3];

        res.is_empty = false;
        return res;
    }

    Instruction execute(Instruction inst) {
        if (inst.is_empty) return inst;
        int res = 0;

        switch (inst.opcode) {
            case 0: case 1: res = inst.op1 + inst.op2 + inst.immediate; break;
            case 3: //loadi
                res = inst.op1; // c++ should automatically sign extend the value, so no need to compute that
                break;
            case 5: res = inst.op1 + inst.op2; break;
            case 7: res = inst.op1 - inst.op2; break;
            case 9: //mul 
                res = (inst.op1 * inst.op2) & 0xFFFFFFFF; // discard upper bits
                break;
            case 20:
                if (evaluateCond(inst.cond, inst.op1, inst.op2)) {
                    program_counter = inst.immediate;
                    // set all earlier stages to empty to squash pipe
                    Instruction emptyInst;
                    emptyInst.is_empty = true;
                    pipeline[STAGE_FETCH] = pipeline[STAGE_DECODE] = pipeline[STAGE_EXECUTE] = emptyInst;
                    // if halt flag has been set, no it hasn't
                    pipeline_halted = false;
                }
                break;
            case 21: //jump
                program_counter = inst.op1;
                // set all earlier stages to empty to squash pipe
                Instruction emptyInst;
                emptyInst.is_empty = true;
                pipeline[STAGE_FETCH] = pipeline[STAGE_DECODE] = pipeline[STAGE_EXECUTE] = emptyInst;
                // if halt flag has been set, no it hasn't
                pipeline_halted = false;
                break;
        }

        inst.result = res;
        return inst;
    }

    Instruction memory(Instruction inst) {
        if (inst.is_empty || inst.type != TYPE_MEMORY) return inst;

        if (inst.opcode == 0) {
            MemoryResult res = memory_system.read(inst.result, STAGE_MEMORY);
            if (res.status == STATUS_DONE) {
                inst.writeback_val = res.value;
                return inst;
            } else {
                cout << "memory for instruction " << inst.addr << "(" << getOperationName(inst.opcode) << ")";
                cout << " missed cache, waiting for RAM" << endl;
                inst.hazard = true;
                return inst;
            }
        } else {
            MemoryResult res = memory_system.write(inst.result, inst.op3, STAGE_MEMORY);
            if (res.status == STATUS_DONE) return inst;
            inst.hazard = true;
            cout << "memory for instruction " << inst.addr << "(" << getOperationName(inst.opcode) << ")";
            cout << " missed cache, waiting for RAM" << endl;
            return inst;
        }
    }

    int writeback(Instruction inst) { // why does this have a return value, it's always FLAG_RUNNING...
        if (inst.is_empty) return FLAG_RUNNING;
        if (inst.type == TYPE_ALU) inst.writeback_val = inst.result;
        if (inst.has_writeback) registers[inst.r0] = inst.writeback_val;
        return FLAG_RUNNING;
    }

    void displayPipeline() {
        cout << "Cycle: " << cycle_count;
        if (pipeline_halted) cout << " [Halted] ";
        cout << endl << " | ";
        for (int i = 0; i < 5; i++)
            cout << getStageDisplay(pipeline[i], i) << " | ";
        cout << endl << endl;
    }

    void viewRegisters() {
        for (int i = 0; i < NUM_REGISTERS; i++)
            cout << "R" << setw(2) << i << ": " << registers[i] << "\n";
    }

    void viewMemory (int level, int line) {
        return memory_system.view(level, line);
    }
};

// simple command line UI, did not get around to using Qt or something more advanced 

// commenting out this main so it does not interfere with Qt implementation


// int main() {
//     Simulator sim;
//     string command;

//     cout << "starting CacheFlow pipeline simulator...\n";

//     while (true) {
//         cout << "\n> enter command (load/run/step/view/reset/exit/viewmem [level] [line]): ";
//         cin >> command;

//         if (command == "load") {
//             string file;
//             cin >> file;
//             sim.loadProgramFromFile(file);
//             cout << "Program loaded.\n";
//         } else if (command == "run") {
//             int cycles;
//             cin >> cycles;
//             for (int i = 0; i < cycles; i++) {
//                 if (sim.step() == FLAG_HALT) {
//                     cout << "Program halted.\n";
//                     break;
//                 }
//                 sim.displayPipeline();
//             }
//         } else if (command == "step") {
//             if (sim.step() == FLAG_HALT) {
//                 cout << "Program halted.\n";
//             }
//             sim.displayPipeline();
//         } else if (command == "view") {
//             sim.viewRegisters();
//         } else if (command == "reset") {
//             sim = Simulator();
//             cout << "Simulator reset.\n";
//         } else if (command == "exit") {
//             break;
//         } else if (command == "viewmem") {
//             int level, line;
//             cin >> level >> line;
//             sim.viewMemory(level, line);
//         } else {
//             cout << "Unknown command.\n";
//         }
//     }

//     return 0;
// }
