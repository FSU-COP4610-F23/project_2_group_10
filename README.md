# project_2_group_10
Project 2: Division of Labor
Operating Systems

Team Members:
1. Fernando Parra: fap20g@fsu.edu – fap20g
2. Roderick Shaw: rks21b@fsu.edu – rks21b
3. Sofia Sanchez: srs20h@fsu.edu – srs20h

Part 1: System Call Tracing
- Roderick Shaw, Fernando Parra

Part 2: Timer Kernel Module
- Roderick Shaw

Part 3a: Adding System Calls
- Roderick Shaw, Fernando Parra, Sofia Sanchez

Part 3b: Kernel Compilation
- Roderick Shaw, Fernando Parra, Sofia Sanchez

Part 3c: Threads
- Sofia Sanchez

Part 3d: Linked List
- Roderick Shaw, Fernando Parra, Sofia Sanchez

Part 3e: Mutexes
- Roderick Shaw, Sofia Sanchez

Part 3f: Scheduling Algorithm
- Sofia Sanchez

List of Files:
```
│
├── part 1/
│ |── empty.c
│ |── empty.trace
│ |── part1.c
| └── part1.trace
├── part 2/
│ |── src/
| |   └──my_timer.c
│ └── Makefile
├── part 3/
│ |── src/
| |   └──elevator.c
│ |── Makefile
| └── syscalls.c
├── Makefile
├── README.md
```

## How to Compile & Execute

### Requirements
- **Compiler**: gcc -std=c99 nameOfFile -o whatYouWantTheExecutableToBeNamed
- **Dependencies**: None needed to be downloaded

### Compilation
```bash
make all
```
This will build both executables with the makefile and shoves the appropriate
contents into the other folders in this directory. You must create the bin,
obj, and obj2 folders in the same directory.
### Execution
```bash
make run
./bin/shell
```
This will run the shell program and start off with a slightly different prompt than
before. However, this will not create the mytimeout executable. Use make all and then
bin/shell to get back into the shell executable.
  
