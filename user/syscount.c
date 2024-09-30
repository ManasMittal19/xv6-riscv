#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h" // for specifying the maximum number of arguments in the exec system call MAXARG = 32
int find_syscall_from_mask(int mask) {
    // Iterate through each bit (0 to 31) to find which bit is set
    for (int i = 0; i < 32; i++) {
        if (mask == (1 << i)) {
            return i;  // Return the index (syscall number) corresponding to the set bit
        }
    }
    // If no valid bit is found, return -1 (invalid mask)
    return -1;
}


const char *syscall_names[] = {
    "", // Index 0, not used
    "fork",     // SYS_fork
    "exit",     // SYS_exit
    "wait",     // SYS_wait
    "pipe",     // SYS_pipe
    "read",     // SYS_read
    "kill",     // SYS_kill
    "exec",     // SYS_exec
    "fstat",    // SYS_fstat
    "chdir",    // SYS_chdir
    "dup",      // SYS_dup
    "getpid",   // SYS_getpid
    "sbrk",     // SYS_sbrk
    "sleep",    // SYS_sleep
    "uptime",   // SYS_uptime
    "open",     // SYS_open
    "write",    // SYS_write
    "mknod",    // SYS_mknod
    "unlink",   // SYS_unlink
    "link",     // SYS_link
    "mkdir",    // SYS_mkdir
    "close",    // SYS_close
    "getSysCount" // SYS_getSysCount
};

int
main(int argc, char *argv[])
{
    int i; 
    char * command[MAXARG]; // creating the command to be run using exec
    // syscount <mask> command [args] 
    if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')) // need atleast 3 or more arguments, valid mask required
    {
        fprintf(2, "usage: syscount mask command\n"); // printing the correct execution statement
        exit(1);
    }
    uint64 mask = atoi(argv[1]);
    
    // creating our command 
    int pid = fork();
    if(pid == 0)
    {
        for(i = 2; i < argc && i < MAXARG; i++){
            command[i-2] = argv[i];
        }
        command[i-2] = 0;
        exec(command[0], command);
        fprintf(2, "exec failed\n");
        exit(1);
    }
    else 
    {
        
        wait(0);
        int sys_call_for_mask =  find_syscall_from_mask(mask);
        
        int count = getSysCount(sys_call_for_mask);
        //  printf("Syscall number for mask %d: %d\n", mask, sys_call_for_mask); // Debug output
        // printf("Syscall count: %d\n", count); 
        if (count >= 0) {
            printf("PID %d called syscall %s %d times.\n", pid, syscall_names[sys_call_for_mask] , count);
             exit(0);
        } else {
            printf("Error retrieving syscall count\n");
             exit(1);
        }  
    }
    exit(0);
}