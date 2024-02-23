# ccs-1b-mips-emulator

## Usage
1. Clone the repository
2. Place your hex program in `program.txt`
3. Run the emulator using `$ ./emulate`
4. View results!

Note: `archive.txt` contains other programs used for testing.

## Demo

Below is a video of me emulating a program that outputs even integers from 0 to 20. I chose this example because it makes use of `j` and `bne`, a simple test of my program counter implementation.

https://github.com/rohildshah/ccs-1b-emulator/assets/47135708/2f451714-6f63-4e22-9731-d056150a0819

Below is another video of me emulating a program that recursively outputs the sum of the integers from 1 to n, where n is read from user input. I chose this example because it makes use of `syscall`, `jal`, `jr`, `lw`, `sw`, a more involved test of my stack/memory implementation (especially `$ra`).

https://github.com/rohildshah/ccs-1b-emulator/assets/47135708/06a11f6e-6646-43d0-9a13-7fe47ceed3a3

## Implementation details

There's plenty to say, but here are three of the most interesting/critical pieces:

1. I chose to make memory byte-addressable (as in MIPS), and arbitrarily allocated 8 KB. I initialized my stack pointer to the top, and since I don't define dynamic/static/reserved memory domains, the stack is free to expand to the entire 8 KB (slightly less than this because of the instructions themselves).
<img width="582" alt="Screenshot 2024-02-22 at 4 24 14 PM" src="https://github.com/rohildshah/ccs-1b-emulator/assets/47135708/9912c048-cf01-49d4-8681-101340c8a373">

2. The 4-byte instructions are streamed into memory from the input file starting from index 0x0, using `tp`, to keep track of the next available byte in memory.
<img width="437" alt="Screenshot 2024-02-22 at 4 29 58 PM" src="https://github.com/rohildshah/ccs-1b-emulator/assets/47135708/4ffd2a87-4ed5-463c-b8f3-7eb158c041fa">

3. The fetch/execute cycle a) reads `pc`, `pc + 1`, `pc + 2`, `pc + 3` into a single instruction, then b) does high-level dispatching of the instruction to the appropriate handlers.
<img width="437" alt="Screenshot 2024-02-22 at 4 29 58 PM" src="https://github.com/rohildshah/ccs-1b-emulator/assets/47135708/6d325b82-0bd8-4b64-b977-c39d7e8e6ea5">
