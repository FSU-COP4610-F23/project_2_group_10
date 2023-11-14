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
├── README.md
```

## How to Compile & Execute

### Requirements
- **Compiler**: gcc -std=c99 nameOfFile -o whatYouWantTheExecutableToBeNamed
- **Dependencies**: None needed to be downloaded

### Compilation
```bash
make
```
This will build the modules you need to run the part 2 and part 3 program. However, you need
to execute this command in both directories because they have different makefiles.
### Execution
```bash
sudo insmod your_module.ko
execute your command here
sudo rmmod your_module.ko
```
For part 2, run this for a basic demonstration:
```
cat /proc/elevator
```
For part 3, run this for the full elevator:
```
watch -n 1 cat /proc/elevator
```
 - Then, pull up a new terminal, go to the producer-consumer executables, and execute these commands:
```
./producer urNumOfStudents
./consumer --start
./consumer --stop
```
  
