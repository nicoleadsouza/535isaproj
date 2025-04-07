#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include "memoryUI.cpp"

using namespace std;

enum Stage { FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK };

struct Instruction {
    uint32_t raw = 0;
    int pc = 0;
    bool valid = false;
};

class PipelineSimulator {
private:
    MemorySystem memory;
    vector<Instruction> pipeline;
    vector<int> registers;
    int pc = 0;
    int cycle_count = 0;
    bool running = false;
    int breakpoint = -1;

public:
    PipelineSimulator() : pipeline(5), registers(32, 0) {}

    void reset() {
        pc = 0;
        cycle_count = 0;
        running = false;
        registers.assign(32, 0);
        pipeline = vector<Instruction>(5);
        memory.reset();
        cout << "Simulator reset.\n";
    }

    void load_program(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file) {
            cout << "Failed to open program.\n";
            return;
        }
        int addr = 0;
        uint32_t instr;
        while (file.read(reinterpret_cast<char*>(&instr), sizeof(instr))) {
            memory.write(addr++, instr, 3);  // Use stage 3 for write
        }
        cout << "Program loaded.\n";
    }

    void step() {
        cycle_count++;

        // WB stage
        if (pipeline[WRITEBACK].valid) {
            uint32_t inst = pipeline[WRITEBACK].raw;
            int opcode = (inst >> 26) & 0x3F;
            int rd = (inst >> 16) & 0x1F;
            int result = registers[rd];  // would be updated in EX/MEM stages
            if (opcode == 0x00 || opcode == 0x01 || opcode == 0x02) {
                cout << "WB: R" << rd << " = " << result << endl;
            }
        }

        // MEM stage
        if (pipeline[MEMORY].valid) {
            uint32_t inst = pipeline[MEMORY].raw;
            int opcode = (inst >> 26) & 0x3F;
            int rs1 = (inst >> 21) & 0x1F;
            int rd = (inst >> 16) & 0x1F;
            int imm = inst & 0x7FF;

            if (opcode == 0x02) {  // LOAD
                auto [done, val] = memory.read(registers[rs1] + imm, MEMORY);
                if (done) registers[rd] = val;
            } else if (opcode == 0x03) {  // STORE
                memory.write(registers[rs1] + imm, registers[rd], MEMORY);
            }
        }

        // EXECUTE stage
        if (pipeline[EXECUTE].valid) {
            uint32_t inst = pipeline[EXECUTE].raw;
            int opcode = (inst >> 26) & 0x3F;
            int rs1 = (inst >> 21) & 0x1F;
            int rs2 = (inst >> 16) & 0x1F;
            int rd = (inst >> 11) & 0x1F;
            int imm = inst & 0x7FF;

            if (opcode == 0x00) registers[rd] = registers[rs1] + registers[rs2]; // ADD
            else if (opcode == 0x01) registers[rd] = registers[rs1] + imm;       // ADDI
        }

        // ID stage: no-op for now

        // IF stage: fetch next instruction
        auto [done, val] = memory.read(pc, FETCH);
        Instruction fetched;
        if (done) {
            fetched.raw = val;
            fetched.pc = pc;
            fetched.valid = true;
            pc++;
        }

        // shift pipeline
        for (int i = WRITEBACK; i > 0; i--) pipeline[i] = pipeline[i - 1];
        pipeline[0] = fetched;
    }

    void run(int n = -1) {
        running = true;
        int count = 0;
        while (running && (n == -1 || count < n)) {
            step();
            count++;
            if (breakpoint != -1 && pc == breakpoint) {
                cout << "Hit breakpoint at PC=" << pc << "\n";
                break;
            }
        }
    }

    void set_breakpoint(int addr) {
        breakpoint = addr;
        cout << "Breakpoint set at PC=" << addr << "\n";
    }

    void print_state() {
        cout << "Cycle: " << cycle_count << " | PC: " << pc << "\n";
        for (int i = 0; i < 8; i++) {
            cout << "R" << i << ": " << registers[i] << "\t";
            if (i % 4 == 3) cout << "\n";
        }
    }
};

// -------- simple command line UI --------

int main() {
    PipelineSimulator sim;
    string cmd;

    cout << "Simulator ready. Type commands:\n";

    while (cin >> cmd) {
        if (cmd == "load") {
            string file;
            cin >> file;
            sim.load_program(file);
        } else if (cmd == "run") {
            int cycles;
            if (cin >> cycles) sim.run(cycles);
            else sim.run();
        } else if (cmd == "step") {
            sim.step();
        } else if (cmd == "bp") {
            int bp;
            cin >> bp;
            sim.set_breakpoint(bp);
        } else if (cmd == "state") {
            sim.print_state();
        } else if (cmd == "reset") {
            sim.reset();
        } else if (cmd == "exit") {
            break;
        } else {
            cout << "Unknown command.\n";
        }
    }
    return 0;
}
