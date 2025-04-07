#include <iostream>
using namespace std;

struct instruction {
    int opcode;
    int r1;
    int r2;
    int r3;
    int imm;
};

int main() {
    /* * ```LOAD R1, [R0 + 0]```

    * ```ADD R2, R1, R1```

    * ```STR R2, [R0 + 0]```*/

    int load = 0;
    int add = 5;
    int str = 1;

    instruction load_inst;
    load_inst.opcode = load;
    load_inst.r1 = 1;
    load_inst.r2 = 0;
    load_inst.r3 = 0;
    load_inst.imm = 0;

    instruction add_inst;
    add_inst.opcode = add;
    add_inst.r1 = 2;
    add_inst.r2 = 1;
    add_inst.r3 = 1;
    add_inst.imm = 0;

    instruction str_inst;
    str_inst.opcode = str;
    str_inst.r1 = 2;
    str_inst.r2 = 0;
    str_inst.r3 = 0;
    str_inst.imm = 0;

    unsigned int load_binary = (load_inst.opcode << 27) | (load_inst.r1 << 23) | (load_inst.r2 << 19) | (load_inst.r3 << 15) | load_inst.imm;
    unsigned int add_binary = (add_inst.opcode << 27) | (add_inst.r1 << 23) | (add_inst.r2 << 19) | (add_inst.r3 << 15) | add_inst.imm;
    unsigned int str_binary = (str_inst.opcode << 27) | (str_inst.r1 << 23) | (str_inst.r2 << 19) | (str_inst.r3 << 15) | str_inst.imm;

    cout << "load: " << load_binary << endl;
    cout << "add: " << add_binary << endl;
    cout << "store: " << str_binary << endl;
}