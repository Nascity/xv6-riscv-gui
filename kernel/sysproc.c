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
sys_draw_fill(void)
{
	int x, y, w, h;
	int color;

	argint(0, &x);
	argint(1, &y);
	argint(2, &w);
	argint(3, &h);
	argint(4, &color);

	if (x < 0 || y < 0 || w < 0 || h < 0)
		return -1;
	
	draw_fill(x, y, w, h, color);
	
	return 0;
}

uint64
sys_draw_bits(void)
{
	int x, y, w, h;
	uint64 bits;
	int size;

	argint(0, &x);
	argint(1, &y);
	argint(2, &w);
	argint(3, &h);
	argaddr(4, &bits);
	argint(5, &size);
	
	if (x < 0 || y < 0 || w < 0 || h < 0)
		return -1;

	draw_bits(x, y, w, h, (uint32*)bits, size);
	
	return 0;
}

uint64
sys_send_msg(void)
{
	int pid;
	uint64 user_buf;
	int size;

	argint(0, &pid);
	argaddr(1, &user_buf);
	argint(2, &size);

	return send_msg(pid, user_buf, size);
}

uint64
sys_recv_msg(void)
{
	uint64 user_buf;
	int size;
	int timeout;

	argaddr(0, &user_buf);
	argint(1, &size);
	argint(2, &timeout);

	return recv_msg(user_buf, size, timeout);
}
