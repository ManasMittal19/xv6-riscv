
## Report

# Task 1 : Adding a syscount system call and the corresponding user program 

Step 1 : Add the user program
- Step 1.1 : Create a new file `user/syscount.c`.
This files contains the logic when a user provide `syscount` as the input.
First check if the syntax is valid or not.
If valid create a child and execute the command using exec system call.
The parent wait for the child process to finish.
After this we call the system call `getSysCount()`  to count all the system calls.

- Step 1.2 : Add the system call `getSysCount()` declaration in the `user/user.h` file.
- Step 1.3 : Update the `Makefile` to also include the user program `syscount`.
- Step 1.4 : Make an entry in the `user.pl` file.

Step 2 : Add the system call 

- Step 2.1 :  Add the system call `getSysCount()` in `kernel/sysproc.c`.
- Step 2.2 :  Add the system call number `SYS_getSysCount` in `kernel/syscall.h`.
- Step 2.3 :  Add the function prototype `int getSysCount(int);` in `kernel/syscall.h` and `kernel/sysproc.c`.
- Step 2.4 :  Add the code to increment the syscall count in `kernel/syscall.c` and also declare it `syscalls` array.
- Step 2.5 :  Add a new array `syscall_count` in `proc.h` and update `proc.c` for the array initialization.
- Step 2.6 :  Declare a new global constant for Maximum number of system call in the `param.h` file.
- Step 2.7 :  Change the `wait` system call in `kernel/proc.c` to count the number of times a process has called it by updating its parent..

# Task 2 Adding the sigalarm and sigreturn system call 

Step 1 : Changes in the user folder : we are not creating any explicit user program but it is necessary for alarmtest.c file to execute properly.
- Step 1.1 : Add the definition in the `user.h` file.
- Step 1.2 : Make entries in the `user.pl` file.

Step 2 : Adding the system calls.

- Step 2.1 : 

# Task 3 : Scheduler



Understanding the flow of scheduler code in xv6. 
Scheduling : `proc.c` (primarily)
Context Switch : `swtch.S` -> It is called by  `swtch(struct context * old, struct context * new)` function.

Flow (in normal RR ) : 
A process running after a fixed time interval gives back the CPU using the `yield()` call.
- Step 1 : Add a macro `SCHEDULER` in the Makefile (just like CPUS) and use CFLAG to make it availabe for the C compiler.
- Step 2 : 