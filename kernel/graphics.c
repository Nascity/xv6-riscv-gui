#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "defs.h"
#include "graphics.h"

void testfill(void)
{
	int x, y;

	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++);
}
