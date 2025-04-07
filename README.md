# Demo Setup #

This is a partial simulator that lets you load a sample set of instructions and watch how each instruction moves through the appropriate 5-stage pipeline (i.e. fetch, decode, execute, memory, writeback). You can step through it cycle by cycle, see memory access delays, and view the state of registers along the way.

1. Download ZIP file of this repo via GitHub, and unzip it.

2. Open via VSCode (or whatever it is that you're using).

3. ```test.txt``` is simply a sample file I made for testing/demoing. Please do not change it. The ```basicsimulator.cpp``` loads this file accordingly as it's a list of instructions (which in this case, are decimal representations of 32-bit binary instructions). ```test.txt``` is specifically an encoded version of:

* ```LOAD R1, [R0 + 0]```

* ```ADD R2, R1, R1```

* ```STR R2, [R0 + 0]```

4. Open a terminal window on your machine. I'd suggest doing this outside of VSCode to ensure that the demo focuses on the program running, and not the code distracting it.

5. Make sure you navigate to correct folder/repo.

6. Run ```g++ basicsimulator.cpp -std=c++11 -o simulator```, which compiles the simulator and creates the executable.

7. Run ```./simulator```, which starts up the simulator.

8. Run ```load test.txt```, which will hand the instructions over to the simulator. It should then say ```Program loaded.```

9. Run ```step```, and doing so several times should show the appropriate changes (hopefully).

10. Run commands like ```run 10``` and ```view```, and play around with it to see what happens.

Note: As of right now, I am not entirely sure that the test case is working the way it should be, especially because I wanted to show the decimal addresses within the simple CL-interface. It also just shows "0" for everything when you enter ```view```—cannot understand why, and it was working correctly at one point.
