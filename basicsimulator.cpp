#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include "memoryUI.cpp"

using namespace std;

const int NUM_REGISTERS = 32;
const int STAGE_FETCH = 0;
const int STAGE_DECODE = 1;
const int STAGE_EXECUTE = 2;
const int STAGE_MEMORY = 3;
const int STAGE_WRITEBACK = 4;

const int FLAG_RUNNING = 0;
const int FLAG_HALT = 1;

struct Instruction {
    int addr = -1;
    unsigned int binary = -1;
    int opcode = -1;
    int r0 = -1, r1 = -1, r2 = -1;
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
    int instruction_count = 0;

    vector<Instruction> pipeline = vector<Instruction>(5);

    bool use_pipeline;
    bool keep_fetching = true;

    string getStageDisplay(const Instruction& inst, int stage) const {
        if (inst.is_empty) return "empty";
        if (inst.stall) return "stall";
        if (inst.hazard) return "hazard";
        return "Instr@" + to_string(inst.addr);
    }

public:
    Simulator(bool pipe = true, bool cache = true)
        : registers(NUM_REGISTERS, 0), program_counter(0),
          use_pipeline(pipe), memory_system(cache) {}

    void loadProgramFromFile(const string& filename) {
        ifstream infile(filename);
        unsigned int instr;
        int addr = 0;
        while (infile >> instr) {
            memory_system.forceWrite(addr++, instr);
        }
        program_counter = 0;
        pipeline = vector<Instruction>(5);
        cycle_count = 0;
        instruction_count = 0;
    }

    int step() {
        if (!pipeline[STAGE_WRITEBACK].is_empty) {
            writeback(pipeline[STAGE_WRITEBACK]);
            pipeline[STAGE_WRITEBACK].is_empty = true;
        }

        if (!pipeline[STAGE_MEMORY].is_empty) {
            pipeline[STAGE_WRITEBACK] = memory(pipeline[STAGE_MEMORY]);
            pipeline[STAGE_MEMORY].is_empty = true;
        }

        if (!pipeline[STAGE_EXECUTE].is_empty) {
            pipeline[STAGE_MEMORY] = execute(pipeline[STAGE_EXECUTE]);
            pipeline[STAGE_EXECUTE].is_empty = true;
        }

        if (!pipeline[STAGE_DECODE].is_empty) {
            pipeline[STAGE_EXECUTE] = decode(pipeline[STAGE_DECODE]);
            pipeline[STAGE_DECODE].is_empty = true;
        }

        if (!pipeline[STAGE_FETCH].is_empty) {
            pipeline[STAGE_DECODE] = fetch(pipeline[STAGE_FETCH]);
            pipeline[STAGE_FETCH].is_empty = true;
        }

        if (pipeline[STAGE_FETCH].is_empty && keep_fetching) {
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
        MemoryResult res = memory_system.read(inst.addr, STAGE_FETCH);
        if (res.status == STATUS_DONE) {
            Instruction out;
            out.binary = res.value;
            out.addr = inst.addr;
            out.is_empty = false;
            program_counter++;
            return out;
        } else {
            inst.hazard = true;
            return inst;
        }
    }

    Instruction decode(Instruction inst) {
        Instruction out;
        out.addr = inst.addr;
        out.binary = inst.binary;
        out.opcode = (inst.binary & 0xF8000000) >> 27;
        out.r0 = (inst.binary & 0x07800000) >> 23;
        out.r1 = (inst.binary & 0x00780000) >> 19;
        out.r2 = (inst.binary & 0x00078000) >> 15;
        out.target = out.r0;
        out.op1 = registers[out.r1];
        out.op2 = registers[out.r2];
        out.has_writeback = (out.opcode != 1); // example: store = no writeback
        out.is_empty = false;
        return out;
    }

    Instruction execute(Instruction inst) {
        if (inst.opcode == 5) {
            inst.result = inst.op1 + inst.op2;
        } else if (inst.opcode == 7) {
            inst.result = inst.op1 - inst.op2;
        } else {
            inst.result = inst.op1; // default: pass-through
        }
        return inst;
    }

    Instruction memory(Instruction inst) {
        if (inst.opcode == 0) {
            MemoryResult res = memory_system.read(inst.result, STAGE_MEMORY);
            inst.writeback_val = res.value;
        } else if (inst.opcode == 1) {
            memory_system.write(inst.result, registers[inst.r2], STAGE_MEMORY);
        } else {
            inst.writeback_val = inst.result;
        }
        return inst;
    }

    int writeback(Instruction inst) {
        if (inst.has_writeback) {
            registers[inst.r0] = inst.writeback_val;
            instruction_count++;
        }
        return FLAG_RUNNING;
    }

    // getters for Qt GUI

    int getCycleCount() const { return cycle_count; }
    int getProgramCounter() const { return program_counter; }
    int viewRegister(int reg) const { return registers[reg]; }
    string getStageDisplayText(int stage) const { return getStageDisplay(pipeline[stage], stage); }
    int getInstructionCount() const { return instruction_count; }
    int getCacheHits() const { return memory_system.getHits(); }
    int getCacheMisses() const { return memory_system.getMisses(); }

    void viewMemory(int level, int line) {
        memory_system.view(level, line);
    }
};
