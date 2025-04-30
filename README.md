# Demo Setup #

This is a partial simulator that lets you load a sample set of instructions and watch how each instruction moves through the appropriate 5-stage pipeline (i.e. fetch, decode, execute, memory, writeback). You can step through it cycle by cycle, see memory access delays, and view the state of registers along the way.

1. Download ZIP file of this repo via GitHub, and unzip it.

2. Open via VSCode (or whatever it is that you're using).

3. Open a terminal window on your machine. I'd suggest doing this outside of VSCode to ensure that the demo focuses on the program running, and not the code distracting it.

5. Make sure you navigate to correct folder/repo.

6. Run ```g++ basicsimulator.cpp -std=c++11 -o simulator```, which compiles the simulator and creates the executable.

7. Run ```./simulator```, which starts up the simulator.

8. Pick one of the test instruction files to load. Then, run ```load inserttestnamehere.txt```, which will hand the instructions over to the simulator. It should then say ```Program loaded.```

9. Run ```step```, and doing so several times should show the appropriate changes (hopefully).

10. Run commands like ```run 10``` and ```view```, and play around with it to see what happens.

## Testing Info ##

* ```basicaddsub.txt```: ADD → SUB → HALT (Goal: Validate arithmetic operations and pipeline data dependency stalling.)
* ```loadandadd.txt```: LOAD → ADD → HALT (Goal: Test memory read delay and proper register dependency handling.)
* ```b2b_writeread```: STR → LOAD → HALT (Goal: Memory write + memory read back-to-back.)
* ```condbranchequal.txt```: BRN (if equal) → ADD (conditional) → HALT (Goal: Verify control flow and branch flushing.)
* ```loadarrstore.txt```: LOAD → ADD → STR → HALT (Goal: Load → arithmetic → store (with proper data forwarding and stalling)

## Writing Assembly ##

* for mnemonics and operation syntax, refer to specification document

* to refer to a register, you can use R0...R15 or directly enter an integer (not recommended)
* integers will be parsed as base 16 and should NOT be prefixed with "0x"
* eg. to indicate the number 16, write "10" and NOT "16" or "0x10"

* lines proceeded by # will be ignored by the assembler
* anything after the expected operands will also be ignored
* for legibility it is recommended to separate any inline comments from the operation in some way
* whitespace is meaningless

* define a symbol by writing it at THE BEGINNING of the SAME LINE as the next mnemonic
* symbols refer to the location in memory they refer to, which is usually the line number

output.txt contains an example of assembly


## How to Run the UI ##

1. Make sure this repo is cloned to machine, and support for Qt is installed.
2. ```cd``` into this repo on your machine.
3. Run ```qmake CacheFlowSim.pro```, ```make```, and then ```open CacheFlowSim.app```
