import java.io.FileReader;
import java.util.HashMap;
import java.util.StringTokenizer;
import java.util.Vector;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileWriter;

class Mnemonic {
    public int opcode;
    public InstType type;
    public Mnemonic(int opcode, InstType type) {
        this.opcode = opcode;
        this.type = type;
    }
}

public class Assembler {
    // TODO: implement pseudo ops?
    // TODO: improve error handling

    // private methods to create instructions for each type
    private static int getTypeAInst(int opcode, int r0, int r1, int r2, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | r2 << 15 | imm;
    }

    private static int getTypeBInst(int opcode, int r0, int r1, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | imm;
    }

    private static int getTypeCInst(int opcode, int r0, int r1, int cond, int imm) {
        return opcode << 27 | r0 << 23 | r1 << 19 | cond << 17 | imm;
    }

    private static int getTypeDInst(int opcode, int r0, int imm) {
        return opcode << 27 | r0 << 23 | imm;
    }

    private static int getOperand(String token, HashMap<String, Integer> symbolTable) {
        return symbolTable.containsKey(token) ? symbolTable.get(token) : Integer.parseInt(token, 16);
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

    public static void main(String[] args) throws Exception {
        HashMap<String, Mnemonic> mnemonicTable = initMnemonicTable();
        HashMap<String, Integer> symbolTable = new HashMap<String, Integer>();
        HashMap<Integer, String> reverseSymbolTable = new HashMap<Integer, String>(); // TODO: add this to output
        String inputFileName, outputFileName;
        String inputLine;
        

        if (args.length < 1 || args.length > 2) {
            throw new Exception("Invalid arguments.");
        }

        inputFileName = args[0];
        if (args.length == 2) {
            outputFileName = args[1];
        } else {
            outputFileName = "output.txt"; // TODO: decide on default filename
        }

        // add register names to symbol table
        for (int i = 0; i < 16; i++) {
            symbolTable.put("R" + i, i);
        }

        int curLocation = 0;

        BufferedReader reader = new BufferedReader(new FileReader(inputFileName));
        // first pass, generate symbol table
        while ((inputLine = reader.readLine()) != null) {
            StringTokenizer line = new StringTokenizer(inputLine, " ");
            Vector<String> tokens= new Vector<String>();
            int numTokens = line.countTokens();

            for (int tokenNumber = 0; tokenNumber < numTokens; tokenNumber++ ) {
                tokens.addElement(line.nextToken());
            }

            // # indicates comment, so ignore lines beginning with it
            if (tokens.size() < 1 || tokens.elementAt(0).equals("#")) continue;

            if (!mnemonicTable.containsKey(tokens.elementAt(0))) {
                System.out.println("Found symbol declaration: " + tokens.elementAt(0));
                System.out.println("Current location: " + curLocation);

                // if not already in symbol table, add it
                if (!symbolTable.containsKey(tokens.elementAt(0))) {
                    symbolTable.put(tokens.elementAt(0), curLocation);
                    reverseSymbolTable.put(curLocation, tokens.elementAt(0));
                }
                tokens.remove(0);
            }

            // for now, only considering instructions and not pseudo ops, so just increment location by one
            curLocation++;
        }

        // reset for second pass
        reader.close();
        BufferedReader secondReader = new BufferedReader(new FileReader(inputFileName));
        curLocation = 0;

        // open output file
        BufferedWriter writer = new BufferedWriter(new FileWriter(outputFileName));

        // second pass, generate instructions
        while ((inputLine = secondReader.readLine()) != null) {
            StringTokenizer line = new StringTokenizer(inputLine, " ");
            Vector<String> tokens= new Vector<String>();
            int numTokens = line.countTokens();

            for (int tokenNumber = 0; tokenNumber < numTokens; tokenNumber++ ) {
                tokens.addElement(line.nextToken());
            }

            // # indicates comment, so ignore lines beginning with it, or empty lines
            if (tokens.size() < 1 || tokens.elementAt(0).equals("#")) continue;

            if (!mnemonicTable.containsKey(tokens.elementAt(0))) {
                tokens.remove(0);
            }

            Mnemonic inst = mnemonicTable.get(tokens.elementAt(0));
            int opcode = inst.opcode;
            int encodedInst, r0, r1, r2, cond, imm;
            switch (inst.type) {
                case InstType.TYPEA:
                    r0 = getOperand(tokens.elementAt(1), symbolTable);
                    r1 = getOperand(tokens.elementAt(2), symbolTable);
                    r2 = getOperand(tokens.elementAt(3), symbolTable);
                    imm = getOperand(tokens.elementAt(4), symbolTable);
                    encodedInst = getTypeAInst(opcode, r0, r1, r2, imm);
                    break;
                case InstType.TYPEB:
                    r0 = getOperand(tokens.elementAt(1), symbolTable);
                    r1 = getOperand(tokens.elementAt(2), symbolTable);
                    imm = getOperand(tokens.elementAt(3), symbolTable);
                    encodedInst = getTypeBInst(opcode, r0, r1, imm);
                    break;
                case InstType.TYPEC:
                    r0 = getOperand(tokens.elementAt(1), symbolTable);
                    r1 = getOperand(tokens.elementAt(2), symbolTable);
                    cond = getOperand(tokens.elementAt(3), symbolTable);
                    imm = getOperand(tokens.elementAt(4), symbolTable);
                    encodedInst = getTypeCInst(opcode, r0, r1, cond, imm);
                    break;
                case InstType.TYPED:
                    r0 = getOperand(tokens.elementAt(1), symbolTable);
                    imm = getOperand(tokens.elementAt(2), symbolTable);
                    encodedInst = getTypeDInst(opcode, r0, imm);
                    break;
                default:
                    encodedInst = -1;
            }

            System.out.println("Instruction at " + curLocation + ": " + tokens);
            System.out.println("Encoded: " + encodedInst);

            // write to output
            writer.write(encodedInst + "\n");

            // for now, only considering instructions and not pseudo ops, so just increment location by one
            curLocation++;
        }

        secondReader.close();
        writer.close();

        // write to file
    }
};