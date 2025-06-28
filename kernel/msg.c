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

// error codes
#define MSG_Q_OK		0
#define MSG_Q_PROC_NOT_FOUND	-1
#define MSG_Q_EMPTY		-2
#define MSG_Q_FULL		-3
#define MSG_Q_TIMEOUT		-4
#define MSG_Q_UNKNOWN		-5

// timeout
#define TIMEOUT		-1
#define INFINITE	-1

static int read_wait(struct proc*, int);
static void write_wait(struct proc*);

extern uint ticks;

int send_msg(struct proc *p, char *kernel_buf, int size, int isalloced)
{
	// acquire write lock
	acquire(&p->write_lock);
	acquire(&p->write_lock2);
	
	// wait for the buffer to be empty
	write_wait(p);
	
	// allocate new queue when empty
	if (!p->msg_queue)
	{
		p->msg_queue = (uint64)kalloc();
		if (!p->msg_queue)
			panic("send_msg - p->msg_queue alloc failed");
	}
	// move kernel memory to queue
	struct msg *pm = &((struct msg*)p->msg_queue)[p->writeptr];
	memset(pm->msg, 0, size);
	memmove(pm->msg, kernel_buf, size);
	pm->size = size;

	// increment writeptr
	p->writeptr = (p->writeptr + 1) % Q_SZ;
	p->justread = 0;

	if (isalloced)
		kfree(kernel_buf);
	release(&p->lock);
	release(&p->write_lock2);
	release(&p->write_lock);
	return MSG_Q_OK;
}

// timeout is in seconds
struct msg *recv_msg(struct proc *p, char *kernel_buf, int size, int timeout)
{
	acquire(&p->read_lock);
	acquire(&p->read_lock2);
	if (!read_wait(p, timeout))
	{
		release(&p->read_lock2);
		release(&p->read_lock);
		return (struct msg*)MSG_Q_TIMEOUT;
	}

	struct msg *pm = &((struct msg*)p->msg_queue)[p->readptr];

	// increment readptr
	p->readptr = (p->readptr + 1) % Q_SZ;
	p->justread = 1;

	return pm;
}

int read_wait(struct proc *p, int timeout)
{
	uint start;

	// set 'start' atomically
	start = ticks;
	__sync_synchronize();

	while (!p->msg_queue || p->readptr == p->writeptr)
	{
		__sync_synchronize();
		if (ticks - start >= timeout * TPS)
			return 0;
		__sync_synchronize();
	}
	__sync_synchronize();
	acquire(&p->lock);
	__sync_synchronize();

	return 1;
}

void write_wait(struct proc *p)
{
	while (1)
	{
		if (p->readptr == (p->writeptr + 1 % Q_SZ))
		{
			if (!p->justread)
				break;
		}
		else
			break;
	}
	__sync_synchronize();
	acquire(&p->lock);
	__sync_synchronize();
}
