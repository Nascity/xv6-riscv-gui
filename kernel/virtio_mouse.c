//
// driver for qemu's virtio mouse device
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"
#include "graphics.h"
#include "mouse.h"

// the address of virtio mmio register r.
#define R(r)	((volatile uint32 *)(PCIE_BASE + (1 << 15) + (r)))

static struct mouse
{
	// lock for mouse operation
	struct spinlock mouse_lock;

	// the location of the mouse pointer
	uint32 x;
	uint32 y;
} mouse;

void
virtio_mouse_init(void)
{
	uint16 vendor = (uint16)*R(PCIE_MMIO_VENDOR);
	uint16 device = (uint16)*R(PCIE_MMIO_DEVICE);

	initlock(&mouse.mouse_lock, "mouse");

	if (vendor != 0x1AF4 || device != 0x1052)
	{
		printf("%x %x\n", vendor, device);
		panic("could not find virtio mouse");
	}
}
