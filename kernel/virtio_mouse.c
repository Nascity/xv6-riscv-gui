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
#define R(r)	((volatile uint32 *)(VIRTIO2 + (r)))

static struct mouse
{
	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;

	// lock for mouse operation
	struct spinlock mouse_lock;

	// the location of the mouse pointer
	uint32 x;
	uint32 y;

	uint32 used_idx;

	struct virtio_input_event evbuf[NUM];
} mouse;

void
virtio_mouse_init(void)
{
	uint32 magic = *R(VIRTIO_MMIO_MAGIC_VALUE);
	uint32 device = *R(VIRTIO_MMIO_DEVICE_ID);
	uint32 vendor = *R(VIRTIO_MMIO_VENDOR_ID);

	initlock(&mouse.mouse_lock, "mouse");

	// checking for vendor and device ids
	if (magic != 0x74726976 || vendor != 0x554d4551 || device != 18)
	{
		printf("%x %x %x\n", magic, vendor, device);
		panic("could not find virtio mouse");
	}

	// reset the device
	*R(VIRTIO_MMIO_STATUS) = 0;

	// set features
	*R(VIRTIO_MMIO_STATUS) = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK;

	// initialize queue 0
	*R(VIRTIO_MMIO_QUEUE_SEL) = 0;

	// ensure queue 0 is not in use
	if (*R(VIRTIO_MMIO_QUEUE_READY))
		panic("virtio mouse should not be ready");

	// check maximum queue size
	uint max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max < NUM)
		panic("virtio mouse max queue too short");

	// allocate and zero queue memory
	mouse.desc = kalloc();
	mouse.avail = kalloc();
	mouse.used = kalloc();
	if (!mouse.desc || !mouse.avail || !mouse.used)
		panic("virtio mouse kalloc");
	memset(mouse.desc, 0, PGSIZE);
	memset(mouse.avail, 0, PGSIZE);
	memset(mouse.used, 0, PGSIZE);

	// set queue size
	*R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	// write physical address
	*R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)mouse.desc;
	*R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)mouse.desc >> 32;
	*R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)mouse.avail;
	*R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)mouse.avail >> 32;
	*R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)mouse.used;
	*R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)mouse.used >> 32;

	//// MAYBE DELETE?
	for (int i = 0; i < NUM; i++)
	{
		mouse.desc[i].addr = (uint64)&mouse.evbuf[i];
		mouse.desc[i].len = sizeof(struct virtio_input_event);
		mouse.desc[i].flags = VRING_DESC_F_WRITE;
		mouse.desc[i].next = 0;
	}

	for (int i = 0; i < NUM; i++)
	{
		mouse.avail->ring[i] = i;
		__sync_synchronize();
		mouse.avail->idx++;
		__sync_synchronize();
	}
	// queue is ready
	*R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

	*R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

	mouse.used_idx = 0;

	printf("mouse initialized\n");
}

void
virtio_mouse_intr(void)
{
	acquire(&mouse.mouse_lock);

	*R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
	__sync_synchronize();

	printf("used->idx = %d\n", mouse.avail->idx);
	while (mouse.used_idx != mouse.avail->idx)
	{
		__sync_synchronize();
		int id = mouse.used->ring[mouse.used_idx].id;

		struct virtio_input_event *event =
			(struct virtio_input_event*)&mouse.desc[id];

		printf("type; %d code: %d value: %d\n", event->type, event->code, event->value);

		mouse.used_idx++;
	}

	release(&mouse.mouse_lock);
}
