#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iomanip>
#include "memoryUI.cpp" // includes memory system with cache and ram

using namespace std;

// constants for simulator setup
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

// structure to represent an instruction in the pipeline
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

// main simulator class
class Simulator {
private:
    vector<int> registers; // register file
    int program_counter;
    MemorySystem memory_system;
    int cycle_count = 0;
    bool pipeline_halted = false;
    vector<Instruction> pipeline = vector<Instruction>(5); // 5-stage pipeline

    // evaluates condition codes for branch instructions
    bool evaluateCond(int cond, int op1, int op2) {
        switch (cond) {
            case 0: return op1 == op2;
            case 1: return op1 < op2;
            case 2: return op1 >= op2;
            case 3: return op1 != op2;
            default: return false;
        }
    }

    // returns string to show current instruction at a pipeline stage
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
    // constructor initializes registers and pc
    Simulator() : registers(NUM_REGISTERS, 0), program_counter(0) {}

    // loads a program (list of binary ints) into memory
    void loadProgramFromFile(const string& filename) {
        ifstream infile(filename);
        unsigned int instr;
        int addr = 0;
        while (infile >> instr) {
            memory_system.write(addr, instr, STAGE_MEMORY);
            addr++;
        }
        program_counter = 0;
        pipeline = vector<Instruction>(5); // clear pipeline
    }

    // performs one clock cycle of pipeline operation
    int step() {
        // writeback stage
        if (!pipeline[STAGE_WRITEBACK].is_empty)
            if (writeback(pipeline[STAGE_WRITEBACK]) == FLAG_HALT)
                return FLAG_HALT;
        pipeline[STAGE_WRITEBACK].is_empty = true;

        // memory stage
        if (!pipeline[STAGE_MEMORY].is_empty) {
            Instruction res = memory(pipeline[STAGE_MEMORY]);
            pipeline[STAGE_WRITEBACK] = res;
            if (!res.stall) pipeline[STAGE_MEMORY].is_empty = true;
        }

        // execute stage
        if (!pipeline[STAGE_EXECUTE].is_empty) {
            Instruction res = execute(pipeline[STAGE_EXECUTE]);
            pipeline[STAGE_MEMORY] = res;
            if (!res.stall) pipeline[STAGE_EXECUTE].is_empty = true;
        }

        // decode stage
        if (!pipeline[STAGE_DECODE].is_empty) {
            Instruction res = decode(pipeline[STAGE_DECODE]);
            pipeline[STAGE_EXECUTE] = res;
            if (!res.stall) pipeline[STAGE_DECODE].is_empty = true;
        }

        // fetch stage
        if (!pipeline[STAGE_FETCH].is_empty) {
            Instruction res = fetch(pipeline[STAGE_FETCH]);
            pipeline[STAGE_DECODE] = res;
            if (!res.stall) pipeline[STAGE_FETCH].is_empty = true;
        }

        // fetch new instruction if fetch stage is empty
        if (pipeline[STAGE_FETCH].is_empty && !pipeline_halted)
            pipeline[STAGE_FETCH] = {addr: program_counter, is_empty: false};

        cycle_count++;
        return FLAG_RUNNING;
    }

    // fetches instruction from memory
    Instruction fetch(Instruction inst) {
        if (inst.is_empty) return inst;
        MemoryResult res = memory_system.read(inst.addr, STAGE_FETCH);
        if (res.status == STATUS_DONE) {
            program_counter++;
            return {binary: (unsigned int)res.value, is_empty: false};
        } else {
            inst.stall = true;
            return inst;
        }
    }

    // decodes binary instruction into fields
    Instruction decode(Instruction inst) {
        if (inst.stall || inst.is_empty) return inst;
        if (inst.binary == -1) {
            pipeline_halted = true;
            return {is_empty: true};
        }

        Instruction res;
        int opcode = (inst.binary & 0xF8000000) >> 27;

        if (opcode == 20) { // branch instruction
            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int cond = (inst.binary & 0x00060000) >> 17;
            int imm = inst.binary & 0x0001FFFF;
            res = {type: TYPE_CONTROL, opcode, r0, r1, 0, cond, imm, registers[r0], registers[r1], 0, 0, false, false, false};
        } else {
            int r0 = (inst.binary & 0x07800000) >> 23;
            int r1 = (inst.binary & 0x00780000) >> 19;
            int r2 = (inst.binary & 0x00078000) >> 15;
            int imm = inst.binary & 0x00007FFF;
            int type = (opcode == 0 || opcode == 1) ? TYPE_MEMORY : TYPE_ALU;
            bool has_writeback = opcode != 1;
            res = {type, opcode, r0, r1, r2, 0, imm, registers[r1], registers[r2], 0, 0, has_writeback, false, false};
        }

        return res;
    }

    // performs computation or address generation
    Instruction execute(Instruction inst) {
        if (inst.stall || inst.is_empty) return inst;
        int res = 0;

        switch (inst.opcode) {
            case 0: case 1: res = inst.op1 + inst.immediate; break; // load/store addr
            case 5: res = inst.op1 + inst.op2; break; // add
            case 7: res = inst.op1 - inst.op2; break; // sub
            case 20: // branch
                if (evaluateCond(inst.cond, inst.op1, inst.op2)) {
                    program_counter += inst.immediate;
                    pipeline = vector<Instruction>(5); // squash on branch
                }
                break;
        }

        inst.result = res;
        return inst;
    }

    // memory stage handles load/store
    Instruction memory(Instruction inst) {
        if (inst.stall || inst.is_empty || inst.type != TYPE_MEMORY) return inst;

        if (inst.opcode == 0) { // load
            MemoryResult res = memory_system.read(inst.result, STAGE_MEMORY);
            if (res.status == STATUS_DONE) {
                inst.writeback_val = res.value;
                return inst;
            } else {
                inst.stall = true;
                return inst;
            }
        } else { // store
            MemoryResult res = memory_system.write(inst.result, inst.op1, STAGE_MEMORY);
            if (res.status == STATUS_DONE) return inst;
            inst.stall = true;
            return inst;
        }
    }

    // write result to register file
    int writeback(Instruction inst) {
        if (inst.is_empty) return FLAG_RUNNING;
        if (inst.has_writeback) registers[inst.r0] = inst.writeback_val;
        return FLAG_RUNNING;
    }

    // prints pipeline stage contents
    void displayPipeline() {
        cout << "Cycle: " << cycle_count << " | ";
        for (int i = 0; i < 5; i++)
            cout << getStageDisplay(pipeline[i], i) << " | ";
        cout << endl;
    }

    // displays register values
    void viewRegisters() {
        for (int i = 0; i < NUM_REGISTERS; i++)
            cout << "R" << setw(2) << i << ": " << registers[i] << "\n";
    }
};

// main loop for user interaction
int main() {
    Simulator sim;
    string command;

    cout << "Welcome to the CacheFlow Pipeline Simulator!\n";

    while (true) {
        cout << "\n> Command (load/run/step/view/reset/exit): ";
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
