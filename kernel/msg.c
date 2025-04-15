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
#define MSG_Q_PROC_NOT_FOUND	-1
#define MSG_Q_EMPTY		-2
#define MSG_Q_FULL		-3
#define MSG_Q_TIMEOUT		-4
#define MSG_Q_UNKNOWN		-5

// timeout
#define TIMEOUT		-1
#define INFINITE	-1

struct spinlock ticks_lock;

static int read_wait(struct proc*, int);
static void write_wait(struct proc*);

extern uint ticks;

int send_msg(struct proc *p, char *kernel_buf, int size)
{
	// acquire write lock
	acquire(&p->write_lock);
	
	// wait for the buffer to be empty
	write_wait(p);
	
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
struct msg *recv_msg(struct proc *p, char *kernel_buf, int size, int timeout)
{
	acquire(&p->read_lock);
	if (!read_wait(p, timeout))
	{
		release(&p->read_lock);
		return (struct msg*)MSG_Q_TIMEOUT;
	}

	struct msg *pm = &((struct msg*)p->msg_queue)[p->readptr];

	// increment readptr
	p->readptr = (p->readptr + 1) % Q_SZ;

	// kinda feel dangerous to release here...
	// hope nothing bad happens
	release(&p->lock);
	release(&p->read_lock);
	return pm;
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
