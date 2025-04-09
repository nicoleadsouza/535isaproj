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
    int op1 = -1, op2 = -1;
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

    bool evaluateCond(int cond, int op1, int op2) {
        switch (cond) {
            case 0: return op1 == op2;
            case 1: return op1 < op2;
            case 2: return op1 >= op2;
            case 3: return op1 != op2;
            default: return false;
        }
    }

    string getStageDisplay(Instruction inst, int stage) {
        if (inst.is_empty) return "empty";
        if (inst.stall) return "stall";
        if (inst.hazard) return "hazard";
        if (inst.opcode == -1) return to_string(inst.addr);
        return getOperationName(inst.opcode);
    }

    string getOperationName(int opcode) {
        switch (opcode) {
            case 0: return "LOAD";
            case 1: return "STR";
            case 5: return "ADD";
            case 7: return "SUB";
            case 20: return "BRN";
            default: return "NOP";
        }
    }

public:
    Simulator() : registers(NUM_REGISTERS, 0), program_counter(0) {}

    void loadProgramFromFile(const string& filename) {
        ifstream infile(filename);
        unsigned int instr;
        int addr = 0;
        while (infile >> instr) {
            memory_system.forceWrite(addr, instr);
            addr++;
        }

        // hard coded this here for testing/demoing purposes
        memory_system.forceWrite(100, 42);
        registers[0] = 100;

        program_counter = 0;
        pipeline = vector<Instruction>(5);
    }

    int step() {
        if (!pipeline[STAGE_WRITEBACK].is_empty) {
            if (writeback(pipeline[STAGE_WRITEBACK]) == FLAG_HALT)
                return FLAG_HALT;
            pipeline[STAGE_WRITEBACK].is_empty = true;
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

        if (pipeline[STAGE_FETCH].is_empty && !pipeline_halted) {
            Instruction inst;
            inst.addr = program_counter;
            inst.is_empty = false;
            pipeline[STAGE_FETCH] = inst;
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
    
            if (res.value == 0) {
                // treat binary 0 as HALT signal
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

        if (opcode == 20) {
            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int cond = (inst.binary & 0x00060000) >> 17;
            int imm = inst.binary & 0x0001FFFF;
            res.type = TYPE_CONTROL;
            res.opcode = opcode;
            res.r0 = r0;
            res.r1 = r1;
            res.op1 = r0;
            res.op2 = r1;
            res.cond = cond;
            res.immediate = imm;
            res.has_writeback = false;
        } else {
            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int r2 = (inst.binary & 0x00078000) >> 15;
            int imm = inst.binary & 0x00007FFF;
            res.type = (opcode == 0 || opcode == 1) ? TYPE_MEMORY : TYPE_ALU;
            res.opcode = opcode;
            res.r0 = r0;
            res.r1 = r1;
            res.r2 = r2;
            res.op1 = r1;
            res.op2 = r2;
            res.target = r0;
            res.immediate = imm;
            res.has_writeback = opcode != 1;
        }

        // handle dependencies
        // search for instructions in pipe targeting the operands
        for (int i = STAGE_EXECUTE; i <= STAGE_WRITEBACK; i++) {
            if (!pipeline[i].is_empty && pipeline[i].has_writeback && (pipeline[i].target == res.op1 || pipeline[i].target == res.op2)) {
                cout << "instruction " << res.addr << "(" << getOperationName(res.opcode) << ")";
                cout << " has dependency on instruction " << pipeline[i].addr <<"(" << getOperationName(pipeline[i].opcode) << ")" << endl;
                // dependency is in the pipe, so we need to stall
                res.hazard = true;
                res.is_empty = false;
                return res;
            }
        }
        // no dependencies in pipe, fetch operands
        res.op1 = registers[res.op1];
        res.op2 = registers[res.op2];

        res.is_empty = false;
        return res;
    }

    Instruction execute(Instruction inst) {
        if (inst.is_empty) return inst;
        int res = 0;

        switch (inst.opcode) {
            case 0: case 1: res = inst.op1 + inst.immediate; break;
            case 5: res = inst.op1 + inst.op2; break;
            case 7: res = inst.op1 - inst.op2; break;
            case 20:
                if (evaluateCond(inst.cond, inst.op1, inst.op2)) {
                    program_counter += inst.immediate;
                    pipeline = vector<Instruction>(5);
                }
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
            MemoryResult res = memory_system.write(inst.result, inst.op1, STAGE_MEMORY);
            if (res.status == STATUS_DONE) return inst;
            inst.hazard = true;
            return inst;
        }
    }

    int writeback(Instruction inst) {
        if (inst.is_empty) return FLAG_RUNNING;
        if (inst.type == TYPE_ALU) inst.writeback_val = inst.result;
        if (inst.has_writeback) registers[inst.r0] = inst.writeback_val;
        return FLAG_RUNNING;
    }

    void displayPipeline() {
        cout << "Cycle: " << cycle_count << endl << " | ";
        for (int i = 0; i < 5; i++)
            cout << getStageDisplay(pipeline[i], i) << " | ";
        cout << endl << endl;
    }

    void viewRegisters() {
        for (int i = 0; i < NUM_REGISTERS; i++)
            cout << "R" << setw(2) << i << ": " << registers[i] << "\n";
    }
};

// simple command line UI, did not get around to using Qt or something more advanced 

int main() {
    Simulator sim;
    string command;

    cout << "starting CacheFlow pipeline simulator...\n";

    while (true) {
        cout << "\n> enter command (load/run/step/view/reset/exit): ";
        cin >> command;

        if (command == "load") {
            string file;
            cin >> file;
            sim.loadProgramFromFile(file);
            cout << "Program loaded.\n";
        } else if (command == "run") {
            int cycles;
            cin >> cycles;
            for (int i = 0; i < cycles; i++) {
                if (sim.step() == FLAG_HALT) {
                    cout << "Program halted.\n";
                    break;
                }
                sim.displayPipeline();
            }
        } else if (command == "step") {
            if (sim.step() == FLAG_HALT) {
                cout << "Program halted.\n";
            }
            sim.displayPipeline();
        } else if (command == "view") {
            sim.viewRegisters();
        } else if (command == "reset") {
            sim = Simulator();
            cout << "Simulator reset.\n";
        } else if (command == "exit") {
            break;
        } else {
            cout << "Unknown command.\n";
        }
    }

    return 0;
}
