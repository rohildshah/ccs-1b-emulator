#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
using namespace std;

#define TWENTYSIX_BITS  0x03FFFFFF
#define SIXTEEN_BITS    0x0000FFFF
#define FIVE_BITS       0x0000001F
#define SIX_BITS        0x0000003F

int registers[32];
uint8_t memory[8192]; // 8 KB
int sp = 0x2000; // stack pointer (0x800 = 2048)
int tp = 0; // text pointer (ie pointer to next free byte in text memory)
int pc = 0; // program counter

bool verbose = false;

void syscall_handler() {
    switch (registers[2]) {
        case 1:
            if (verbose) cout << "print integer ..." << endl;
            cout << registers[4] << flush;
            break;
        case 4:
            // needs testing (where would strings be stored in memory?)
            // cout << registers[4] << flush;
            break;
        case 5:
            if (verbose) cout << "read integer ..." << endl;
            cin >> registers[2];
            break;
        case 10:
            if (verbose) cout << "end program ..." << endl;
            pc = tp;
            break;
        default:
            if (verbose) cout << "Unrecognized v0 value!" << endl;
    }
}

void r_type_handler(int instruction) {
    int rs = (instruction >> 21) & FIVE_BITS;
    int rt = (instruction >> 16) & FIVE_BITS;
    int rd = (instruction >> 11) & FIVE_BITS;
    int shamt = (instruction >> 6) & FIVE_BITS;
    int funct = instruction & SIX_BITS;

    switch (funct) {
        case 0x00:
            if (verbose) cout << "sll ..." << endl;
            registers[rd] = registers[rt] << shamt;
            break;
        case 0x08:
            if (verbose) cout << "jr ..." << endl;
            pc = registers[rs];
            break;
        case 0x20:
        case 0x21:
            if (verbose) cout << "add ..." << endl;
            // addu is 0x21, need to check for overflow
            // if (verbose) cout << "addu ..." << endl;
            registers[rd] = registers[rs] + registers[rt];
            break;
        case 0x22:
            if (verbose) cout << "sub ..." << endl;
            // subu is 0x23, need to check for overflow
            // if (verbose) cout << "subu ..." << endl;
            registers[rd] = registers[rs] - registers[rt];
            break;
        case 0x24:
            if (verbose) cout << "and ..." << endl;
            registers[rd] = registers[rs] & registers[rt];
            break;
        case 0x25:
            if (verbose) cout << "or ..." << endl;
            registers[rd] = registers[rs] | registers[rt];
            break;
        case 0x27:
            if (verbose) cout << "nor ..." << endl;
            registers[rd] = ~(registers[rs] | registers[rt]);
            break;
        case 0x2A:
        case 0x2B:
            if (verbose) cout << "slt ..." << endl;
            // sltu is 0x2b, need to implement
            // if (verbose) cout << "sltu ..." << endl;
            registers[rd] = registers[rs] < registers[rt];
            break;
        case 0x03:
            // C++20: right shift of signed value is always arithmetic
            if (verbose) cout << "sra ..." << endl;
            registers[rd] = registers[rs] >> shamt;
            break;
        case 0x0C:
            if (verbose) cout << "syscall: ";
            syscall_handler();
            break;
        default:
            if (verbose) cout << "Unrecognized R-type funct!" << endl;        
    }
}

void i_type_handler(int opcode, int instruction) {
    int rs = (instruction >> 21) & FIVE_BITS;
    int rt = (instruction >> 16) & FIVE_BITS;
    int16_t immediate = instruction & SIXTEEN_BITS;

    switch (opcode) {
        case 0x04:
            if (verbose) cout << "beq ... " << endl;
            // In real MIPS this seems to be pc += immediate + 4 because branch instructions are relative
            // But there is no linker for this code, so just take the absolute address
            if (registers[rs] == registers[rt]) pc = immediate;
            break;
        case 0x05:
            if (verbose) cout << "bne ... " << endl;
            if (registers[rs] != registers[rt]) pc = immediate;
            break;
        case 0x08:
            if (verbose) cout << "addi ... " << endl;
            registers[rt] = registers[rs] + immediate;
            break;
        case 0x0a:
            if (verbose) cout << "slti ... " << endl;
            registers[rt] = registers[rs] < immediate;
            break;
        case 0x0c:
            if (verbose) cout << "andi ... " << endl;
            registers[rt] = registers[rs] & immediate;
            break;
        case 0x0d:
            if (verbose) cout << "ori ... " << endl;
            registers[rt] = registers[rs] | immediate;
            break;
        case 0x0f:
            if (verbose) cout << "lui ... " << endl;
            registers[rt] = immediate << 16;
            break;
        case 0x23: {
            if (verbose) cout << "lw ..." << endl;
            unsigned int word = 0;
            for (int i = 0; i < 4; i++) {
                word = word << 8;
                word += memory[registers[rs] + immediate + i];
            }
            registers[rt] = word;
            break;
        }
        case 0x25:
            if (verbose) cout << "lhu ..." << endl;
            break;
        case 0x2b: {
            if (verbose) cout << "sw ..." << endl;
            unsigned int word = registers[rt];
            for (int i = 0; i < 4; i++) {
                memory[registers[rs] + immediate + i] = word >> 24;
                word = word << 8;
            }
            break;
        }
        default:
            if (verbose) cout << "Unrecognized I-type!" << endl;
    }
}

void j_type_handler(int opcode, int instruction) {
    int address = instruction & TWENTYSIX_BITS;

    switch (opcode) {
        case 0x02:
            if (verbose) cout << "j ..." << endl;
            pc = address;
            break;
        case 0x03:
            if (verbose) cout << "jal ..." << endl;
            // In real MIPS this seems to be +8 because of branch delay slot
            // https://stackoverflow.com/questions/9548927/mips-jal-confusion-ra-pc4-or-pc8
            registers[31] = pc;
            pc = address;
            break;
        default:
            if (verbose) cout << "Unrecognized J-type!" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Incorrect usage: ./a.out <filename>" << endl;
        return 1;
    }

    registers[29] = sp;

    // Places instructions at bottom of memory
    ifstream instructions(argv[1]);
    while (!instructions.eof()) {
        unsigned int instruction;
        instructions >> hex >> instruction;

        // Places instruction 0x0A0B0C0D
        // memory[tp] = 0A, memory[tp + 1] = 0B, etc
        for (int i = 0; i < 4; i++) {
            memory[tp] = instruction >> 24;
            instruction = instruction << 8;
            tp++;
        }
    }

    // Fetch/Execute cycle ends when all instructions are read
    while (pc != tp) {
        // Fetch instruction from memory
        unsigned int instruction = 0;
        for (int i = 0; i < 4; i++) {
            instruction = instruction << 8;
            instruction += memory[pc];
            pc++;
        }

        // Decode & Execute
        int opcode = instruction >> 26;
        switch (opcode) {
            case 0x00:
                r_type_handler(instruction);
                break;
            case 0x02:
            case 0x03:
                // These are the only J-types
                j_type_handler(opcode, instruction);
                break;
            default:
                i_type_handler(opcode, instruction);
                break;
        }
    }

    for (auto reg : registers) {
        if (verbose) cout << reg << endl;
    }
}