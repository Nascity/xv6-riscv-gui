#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"

void initsema(struct semaphore *s, int val)
{
	s->val = val;
	initlock(&s->lock, "sema");
}

void P(struct semaphore *s)
{
	printf("P in! - val: %d\n", s->val);
	while (1)
	{
		while (s->val <= 0);
		__sync_synchronize();
		acquire(&s->lock);		// ac
		__sync_synchronize();
		if (s->val > 0)
		{
			s->val--;
			__sync_synchronize();
			release(&s->lock);	// re
			__sync_synchronize();
			printf("P!\n");
			break;
		}
		__sync_synchronize();
		release(&s->lock);		// re
		__sync_synchronize();
	}
}

void V(struct semaphore *s)
{
	printf("V in! - val: %d\n", s->val);
	acquire(&s->lock);
	__sync_synchronize();
	s->val++;
	__sync_synchronize();
	acquire(&s->lock);
	__sync_synchronize();
	printf("V!\n");
}
