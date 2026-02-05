# microasm
A very small barely functionable assembler for MIPS assembly, written in C.
It outputs the machine code as hexadecimal, and so far only supports to instructions: Jump and Load Word.
You also have to write the fields of the instruction as they are written in machine code.
So far labels aren't supported.

Command line arguments:
1: The file you want to assemble.
2: Where to output the hex code.

Build with *make* and build and run with *make run*
