//
// driver for qemu's virtio gpu device.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "virtio.h"
#include "graphics.h"

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO1 + (r)))

void get_display_info(void);
void virtio_gpu_send(int, void*, int, void*, int);

static struct gpu
{
	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;

	struct spinlock gpu_lock;

	uint32 width;
	uint32 height;
} gpu;

void
virtio_gpu_init(void)
{
	uint32 magic = *R(VIRTIO_MMIO_MAGIC_VALUE);
	uint32 device = *R(VIRTIO_MMIO_DEVICE_ID);
	uint32 vendor = *R(VIRTIO_MMIO_VENDOR_ID);

	initlock(&gpu.gpu_lock, "gpu");

	if (magic != 0x74726976 || vendor != 0x554d4551 || device != 16)
		panic("could not find virtio gpu");
	
	// reset the device
	*R(VIRTIO_MMIO_STATUS) = 0;

	// initialize queue 0
	*R(VIRTIO_MMIO_QUEUE_SEL) = 0;

	// ensure queue 0 is not in use
	if (*R(VIRTIO_MMIO_QUEUE_READY))
		panic("virtio gpu should not be ready");

	// check maximum queue size
	uint max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (max < NUM)
		panic("virtio gpu max queue too short");

	// allocate and zero queue memory
	gpu.desc = kalloc();
	gpu.avail = kalloc();
	gpu.used = kalloc();
	if (!gpu.desc || !gpu.avail || !gpu.used)
		panic("virtio gpu kalloc");
	memset(gpu.desc, 0, PGSIZE);
	memset(gpu.avail, 0, PGSIZE);
	memset(gpu.used, 0, PGSIZE);

	// set queue size
	*R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	// write physical address
	*R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)gpu.desc;
	*R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)gpu.desc >> 32;
	*R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)gpu.avail;
	*R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)gpu.avail >> 32;
	*R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)gpu.used;
	*R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)gpu.used >> 32;

	// queue is ready
	*R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

	get_display_info();
}

void get_display_info(void)
{
	struct virtio_gpu_ctrl_hdr req = {
		.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
		.flags = 0,
		.fence_id = 0,
		.ctx_id = 0,
		.padding = 0
	};

	struct virtio_gpu_resp_display_info resp;

	virtio_gpu_send(0, &req, sizeof(req), &resp, sizeof(resp));

	gpu.width = resp.pmodes[0].r.width;
	gpu.height = resp.pmodes[0].r.height;

	printf("GPU initialized: %dx%d.\n", gpu.width, gpu.height);
}

void virtio_gpu_send(int queue_id, void *cmd, int cmdlen, void *resp, int resplen)
{
	int cmd_idx = 0;
	int resp_idx = 1;

	memset(&gpu.desc[cmd_idx], 0, sizeof(gpu.desc[0]));
	gpu.desc[cmd_idx].addr = (uint64)cmd;
	gpu.desc[cmd_idx].len = cmdlen;
	gpu.desc[cmd_idx].flags = VRING_DESC_F_NEXT;
	gpu.desc[cmd_idx].next = resp_idx;

	memset(&gpu.desc[resp_idx], 0, sizeof(gpu.desc[0]));
	gpu.desc[resp_idx].addr = (uint64)resp;
	gpu.desc[resp_idx].len = resplen;
	gpu.desc[resp_idx].flags = VRING_DESC_F_WRITE;

	gpu.avail->ring[gpu.avail->idx % NUM] = cmd_idx;
	__sync_synchronize();
	gpu.avail->idx++;

	*R(VIRTIO_MMIO_QUEUE_NOTIFY) = queue_id;
	while (gpu.used->idx != gpu.avail->idx) __sync_synchronize();
}
