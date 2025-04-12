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

	// BAR addr
	uint64 bar_addr;

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

	// checking for vendor and device ids
	if (vendor != 0x1AF4 || device != 0x1052)
	{
		printf("%x %x\n", vendor, device);
		panic("could not find virtio mouse");
	}

	// allocating BAR for PCEI device
	uint64 bar_addr = (uint64)kalloc();
	if (!bar_addr)
		panic("failed to allocate memory for BAR");
	*R(PCIE_MMIO_BAR0) = bar_addr;
	mouse.bar_addr = bar_addr;

	// reset the device
	*R(PCIE_MMIO_STATUS) = 0;

	// set features
	*R(PCIE_MMIO_STATUS) = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK;
	if (!(*R(PCIE_MMIO_STATUS) & VIRTIO_CONFIG_S_FEATURES_OK))
		panic("mouse didn't except the features");

	// scan for capabilities
	
}
