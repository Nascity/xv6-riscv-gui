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

struct msg
{
	uint64 size;
	uint8 msg[MAX_MSG];
};

struct spinlock ticks_lock;

static int wait_timeout(struct proc, int, int (*)(struct proc*));
static int buffer_has_space(struct proc*);

extern uint64 ticks;

int send_msg(int target_pid, uint64 user_buf, int size)
{
	struct proc *p;

	// find the struct proc* of the corresponding pid
	p = findproc(target_pid);
	if (!p)
		return MSG_Q_PROC_NOT_FOUND;
	
	// allocate new queue when empty
	acquire(&p->lock);
	if (!p->msg_queue)
	{
		p->msg_queue = (uint64)kalloc();
		if (!p->msg_queue)
			panic("send_msg - not enough memory");
	}
	release(&p->lock);

	// wait for buffer to be empty
	if (!wait_timeout(p, INFINITE, buffer_can_be_written))
		panic("send_msg - this shouldn't happen"); // if the infinite loop somehow breaks

	printf("[DEBUG] sizeof(msg) = %ld\n", sizeof(struct msg));
	printf("[DEBUG] wp = %d, rp = %d\n", p->writeptr, p->readptr);

	release(&p->lock);
	return MSG_Q_OK;
}

// timeout is in seconds
int recv_msg(uint64 user_buf, int size, int timeout)
{
	struct proc *p = myproc();

	if (!wait_timeout(p, timeout, buffer_can_be_read))
	{
		release(p->lock);
		return MSG_Q_TIMEOUT;
	}

	printf("[DEBUG] wp = %d, rp = %d\n", p->writeptr, p->readptr);

	release(&p->lock);
	return MSG_Q_OK;
}

// return 0 when timout, 1 when the buffer is empty
// p->lock has to be release later!!
int wait_timeout(struct proc *p, int timeout, int (*cond)(struct proc*))
{
	uint64 start;

	acquire(&ticks_lock);
	start = ticks;
	release(&ticks_lock);

	acquire(&p->lock);
	while (!cond(p))
	{
		if (timeout == INFINITE)
			continue;

		acquire(&ticks_lock);
		if (ticks - start >= timeout * TPS)
		{
			release(&ticks_lock);
			release(&p->lock);
			return 0;
		}
		release(&ticks_lock);
	}

	return 1;
}

// needs to be used with a lock
int buffer_can_be_read(struct proc *p)
{
	return p->msg_queue && readptr != writeptr;
}

// needs to be used with a lock
int buffer_can_be_written(struct proc *p)
{
	return p->msg_queue && ((writeptr + 1) % Q_SZ != readptr);
}
