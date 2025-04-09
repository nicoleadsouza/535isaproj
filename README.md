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
