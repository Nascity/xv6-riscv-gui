//
// message queue implementation
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define MSG_SZ	128
#define MAX_MSG (MSG_SZ - sizeof(uint64))
#define Q_SZ	(PGSIZE / MAX_MSG)

// error codes
#define MSG_Q_OK		0
#define MSG_Q_EMPTY		-1
#define MSG_Q_FULL		-2
#define MSG_Q_TIMEOUT		-3
#define MSG_Q_UNKNOWN		-4
#define MSG_Q_PROC_NOT_FOUND	-5

// timeout
#define TIMEOUT		-1
#define INFINITE	-1

struct msg
{
	uint64 size;
	uint8 msg[MAX_MSG];
};

struct spinlock ticks_lock;

static int read_wait(struct proc*, int);
static void write_wait(struct proc*);

extern uint ticks;

int send_msg(int target_pid, uint64 user_buf, int size)
{
	struct proc *p;
	char* kernel_buf;

	// find the struct proc* of the corresponding pid
	p = findproc(target_pid);
	if (!p)
		return MSG_Q_PROC_NOT_FOUND;

	// acquire write lock
	acquire(&p->write_lock);
	
	// allocate new queue when empty
	acquire(&p->lock);
	if (!p->msg_queue)
	{
		p->msg_queue = (uint64)kalloc();
		if (!p->msg_queue)
			panic("send_msg - p->msg_queue alloc failed");
	}
	release(&p->lock);
	write_wait(p);

	// move user mem to kernel memory
	kernel_buf = (char*)kalloc();
	if (!kernel_buf)
		panic("send_msg - kernel_buf alloc failed");
	if (copyin(p->pagetable, kernel_buf, user_buf, Q_SZ))
		panic("send_msg - copyin failed");
	
	// move kernel memory to queue
	struct msg *pm = &((struct msg*)p->msg_queue)[p->writeptr];
	memmove(pm->msg, kernel_buf, Q_SZ);
	pm->size = size;

	// increment writeptr
	p->writeptr = (p->writeptr + 1) % Q_SZ;

	kfree(kernel_buf);
	release(&p->lock);
	release(&p->write_lock);
	return MSG_Q_OK;
}

// timeout is in seconds
int recv_msg(uint64 user_buf, int size, int timeout)
{
	struct proc *p = myproc();

	acquire(&p->read_lock);
	if (!read_wait(p, timeout))
	{
		release(&p->read_lock);
		return MSG_Q_TIMEOUT;
	}

	// move kernel memory to user memory
	struct msg *pm = &((struct msg*)p->msg_queue)[p->readptr];
	if (copyout(p->pagetable, user_buf, (char*)pm->msg, pm->size))
		panic("recv_msg - copyout failed");

	// increment readptr
	p->readptr = (p->readptr + 1) % Q_SZ;

	release(&p->lock);
	release(&p->read_lock);
	return MSG_Q_OK;
}

int read_wait(struct proc *p, int timeout)
{
	uint start;

	// set 'start' atomically
	acquire(&ticks_lock);
	start = ticks;
	release(&ticks_lock);

	while (!p->msg_queue || p->readptr == p->writeptr)
	{
		acquire(&ticks_lock);
		if (ticks - start >= timeout * TPS)
		{
			release(&ticks_lock);
			return 0;
		}
		release(&ticks_lock);
	}
	__sync_synchronize();
	acquire(&p->lock);

	return 1;
}

void write_wait(struct proc *p)
{
	while (p->readptr == (p->writeptr + 1 % Q_SZ));
	__sync_synchronize();
	acquire(&p->lock);
}
