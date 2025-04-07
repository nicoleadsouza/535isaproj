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
    int addr = 0;
    unsigned int binary = 0;
    int type = 0;
    int opcode = 0;
    int r0 = 0, r1 = 0, r2 = 0;
    int cond = 0;
    int immediate = 0;
    int op1 = 0, op2 = 0;
    int result = 0;
    int writeback_val = 0;
    bool has_writeback = false;
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
        if (stage == STAGE_FETCH) return to_string(inst.binary);
        switch (inst.opcode) {
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
        if (!pipeline[STAGE_WRITEBACK].is_empty)
            if (writeback(pipeline[STAGE_WRITEBACK]) == FLAG_HALT)
                return FLAG_HALT;
        pipeline[STAGE_WRITEBACK].is_empty = true;

        if (!pipeline[STAGE_MEMORY].is_empty) {
            Instruction res = memory(pipeline[STAGE_MEMORY]);
            pipeline[STAGE_WRITEBACK] = res;
            if (!res.stall) pipeline[STAGE_MEMORY].is_empty = true;
        }

        if (!pipeline[STAGE_EXECUTE].is_empty) {
            Instruction res = execute(pipeline[STAGE_EXECUTE]);
            pipeline[STAGE_MEMORY] = res;
            if (!res.stall) pipeline[STAGE_EXECUTE].is_empty = true;
        }

        if (!pipeline[STAGE_DECODE].is_empty) {
            Instruction res = decode(pipeline[STAGE_DECODE]);
            pipeline[STAGE_EXECUTE] = res;
            if (!res.stall) pipeline[STAGE_DECODE].is_empty = true;
        }

        if (pipeline[STAGE_FETCH].is_empty && !pipeline_halted) {
            Instruction inst;
            inst.addr = program_counter;
            inst.is_empty = false;
            pipeline[STAGE_FETCH] = inst;
        }

        if (!pipeline[STAGE_FETCH].is_empty) {
            Instruction res = fetch(pipeline[STAGE_FETCH]);
            pipeline[STAGE_DECODE] = res;
            if (!res.stall) pipeline[STAGE_FETCH].is_empty = true;
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
            inst.stall = true;
            return inst;
        }
    }    

    Instruction decode(Instruction inst) {
        if (inst.stall || inst.is_empty) return inst;
        if (inst.binary == -1) {
            pipeline_halted = true;
            Instruction halt_inst;
            halt_inst.is_empty = true;
            return halt_inst;
        }

        Instruction res;
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
            res.cond = cond;
            res.immediate = imm;
            res.op1 = registers[r0];
            res.op2 = registers[r1];
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
            res.immediate = imm;
            res.op1 = registers[r1];
            res.op2 = registers[r2];
            res.has_writeback = opcode != 1;
        }

        res.is_empty = false;
        return res;
    }

    Instruction execute(Instruction inst) {
        if (inst.stall || inst.is_empty) return inst;
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
        if (inst.stall || inst.is_empty || inst.type != TYPE_MEMORY) return inst;

        if (inst.opcode == 0) {
            MemoryResult res = memory_system.read(inst.result, STAGE_MEMORY);
            if (res.status == STATUS_DONE) {
                inst.writeback_val = res.value;
                return inst;
            } else {
                inst.stall = true;
                return inst;
            }
        } else {
            MemoryResult res = memory_system.write(inst.result, inst.op1, STAGE_MEMORY);
            if (res.status == STATUS_DONE) return inst;
            inst.stall = true;
            return inst;
        }
    }

    int writeback(Instruction inst) {
        if (inst.is_empty) return FLAG_RUNNING;
        if (inst.has_writeback) registers[inst.r0] = inst.writeback_val;
        return FLAG_RUNNING;
    }

    void displayPipeline() {
        cout << "Cycle: " << cycle_count << " | ";
        for (int i = 0; i < 5; i++)
            cout << getStageDisplay(pipeline[i], i) << " | ";
        cout << endl;
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
