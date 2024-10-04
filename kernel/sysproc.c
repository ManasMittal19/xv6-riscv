#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}
 
uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 
count_syscalls(struct proc *p, int syscall_num) {
    uint64 count = 0;
    // struct proc * np;
    if (p == 0) {
        return count;
    }

    count += p->syscall_count[syscall_num];
    
  //   for(np = proc; np < &proc[NPROC]; np++){
  //   if(np->parent == p && np->state != UNUSED)
  //     count += count_syscalls(np, syscall_num);
  // }
    return count;
}
uint64
sys_getSysCount(void)
{
    int syscall_num;
    argint(0 , &syscall_num);
    struct proc *p = myproc();
    
    
    // Check if the syscall_num is within valid bounds
    if (syscall_num < 0 || syscall_num >= NSYSCALL)
        return -1;
    
    return count_syscalls(p , syscall_num);
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;
  argint(0, &interval);
  argaddr(1, &handler);
  if(interval < 0 || handler < 0)
    return -1;
  
  return proc_sigalarm(interval, handler);
}

uint64
sys_sigreturn(void)
{
  return proc_sigreturn();
}

uint64
sys_settickets(void)
{
  int number;
  argint(0, &number);
  
  if(number <= 0)
    return -1;

  #ifdef SCHED_LBS
    myproc()->tickets = number;
    return number;
  #else
    return -1;
  #endif
}


uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}