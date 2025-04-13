import java.util.HashMap;

class Mnemonic {
    public int opcode;
    public InstType type;
    public Mnemonic(int opcode, InstType type) {
        this.opcode = opcode;
        this.type = type;
    }
}

public class Assembler {
    // TODO: implement pseudo ops

    // private methods to create instructions for each type
    private int getTypeAInst(int opcode, int r0, int r1, int r2, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | r2 << 15 | imm;
    }

    private int getTypeBInst(int opcode, int r0, int r1, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | imm;
    }

    private int getTypeCInst(int opcode, int r0, int r1, int cond, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | cond << 17 | imm;
    }

    private int getTypeDInst(int opcode, int r0, int imm) {
        return opcode << 27 | r0 << 23 | imm;
    }

    // method to populate the mnemonic table
    // to create/update/delete mnemonics, just edit this method
    private static HashMap<String, Mnemonic> initMnemonicTable() {
        HashMap<String, Mnemonic> table = new HashMap<String, Mnemonic>(23);

        table.put("LOAD", new Mnemonic(0x0,InstType.TYPEA));
        table.put("STR", new Mnemonic(0x1,InstType.TYPEA));
        table.put("MOV", new Mnemonic(0x2,InstType.TYPEA));
        table.put("LOADI", new Mnemonic(0x3,InstType.TYPED));
        table.put("LOADIZ", new Mnemonic(0x4,InstType.TYPED));
        table.put("ADD", new Mnemonic(0x5,InstType.TYPEA));
        table.put("ADDI", new Mnemonic(0x6,InstType.TYPEB));
        table.put("SUB", new Mnemonic(0x7,InstType.TYPEA));
        table.put("SUBI", new Mnemonic(0x8,InstType.TYPEB));
        table.put("MUL", new Mnemonic(0x9,InstType.TYPEA));
        table.put("MULI", new Mnemonic(0xA,InstType.TYPEB));
        table.put("DIV", new Mnemonic(0xB,InstType.TYPEA));
        table.put("DIVI", new Mnemonic(0xC,InstType.TYPEB));
        table.put("MOD", new Mnemonic(0xD,InstType.TYPEA));
        table.put("MODI", new Mnemonic(0xE,InstType.TYPEB));
        table.put("SHF", new Mnemonic(0xF,InstType.TYPEC));
        table.put("AND", new Mnemonic(0x10,InstType.TYPEA));
        table.put("OR", new Mnemonic(0x11,InstType.TYPEA));
        table.put("XOR", new Mnemonic(0x12,InstType.TYPEA));
        table.put("NOT", new Mnemonic(0x13,InstType.TYPEA));
        table.put("BRN", new Mnemonic(0x14,InstType.TYPEC));
        table.put("JUMP", new Mnemonic(0x15,InstType.TYPED));
        table.put("SUBJ", new Mnemonic(0x16,InstType.TYPED));

        return table;
    }

    public static void main(String[] args) {
        HashMap<String, Mnemonic> mnemonicTable = initMnemonicTable();

        // first pass, generate symbol table

        // second pass, generate instructions

        // write to file
    }
};