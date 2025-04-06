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

extern pagetable_t kernel_pagetable;

static void virtio_gpu_fetch_display_info(void);
static void virtio_gpu_create_resource(void);
static void virtio_gpu_attach_memory(void);
static void virtio_gpu_transfer(void);
static void virtio_gpu_flush(void);
static void virtio_gpu_scanout(void);

static void virtio_gpu_send(void*, int, void*, int, void*, int);
static void virtio_gpu_check_or_die(char*, struct virtio_gpu_ctrl_hdr*, uint32);

static void write_ready_screen(void);

static struct gpu
{
	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;

	// array of checking whether a descriptor is free
	char free[NUM];

	struct spinlock gpu_lock;

	// the dimension of the screen
	uint32 width;
	uint32 height;

	// the physical address of the frame buffer
	uint64 fb_addr[REQUIRED_PAGE_COUNT];
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

	// set features
	*R(VIRTIO_MMIO_STATUS) = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER | VIRTIO_CONFIG_S_DRIVER_OK;

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

	for (int i = 0; i < NUM; i++)
		gpu.free[i] = 1;

	virtio_gpu_fetch_display_info();
	virtio_gpu_create_resource();
	virtio_gpu_attach_memory();

	write_ready_screen();
	virtio_gpu_scanout();
	virtio_gpu_transfer();
	virtio_gpu_flush();

	printf("GPU initialized: %dx%d.\n", gpu.width, gpu.height);
}

void virtio_gpu_fetch_display_info(void)
{
	struct virtio_gpu_ctrl_hdr req = {
		.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
		.flags = 0,
		.fence_id = 0,
		.ctx_id = 0,
		.padding = 0
	};
	struct virtio_gpu_resp_display_info resp;

	virtio_gpu_send(&req, sizeof(req), 0, 0, &resp, sizeof(resp));
	/*
	virtio_gpu_check_or_die("fetch_display_info", &resp.hdr,
				VIRTIO_GPU_RESP_OK_DISPLAY_INFO);
	*/ // appears that this does not need to be bug-checked

	gpu.width = resp.pmodes[0].r.width;
	gpu.height = resp.pmodes[0].r.height;
}

void virtio_gpu_create_resource(void)
{
	struct virtio_gpu_resource_create_2d req = {
		.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
		.resource_id = 1,
		.format = VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM,
		.width = gpu.width,
		.height = gpu.height,
	};
	struct virtio_gpu_ctrl_hdr resp;

	virtio_gpu_send(&req, sizeof(req), 0, 0, &resp, sizeof(resp));
	virtio_gpu_check_or_die("create_resource", &resp, 0);
}

static struct virtio_gpu_mem_entry entries[REQUIRED_PAGE_COUNT];

void virtio_gpu_attach_memory(void)
{
	struct virtio_gpu_resource_attach_backing req = {
		.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
		.resource_id = 1,
		.nr_entries = 1
	};
	struct virtio_gpu_ctrl_hdr resp;

	for (int i = 0; i < REQUIRED_PAGE_COUNT; i++)
	{
		uint64 fb = (uint64)kalloc();

		if (!fb)
			panic("out of memory by gpu");

		entries[i].addr = fb;
		entries[i].length = PGSIZE;
		entries[i].padding = 0;

		gpu.fb_addr[i] = fb;
	}

	virtio_gpu_send(&req, sizeof(req), entries, sizeof(entries), &resp, sizeof(resp));
	virtio_gpu_check_or_die("attach_memory", &resp, 0);
}

void virtio_gpu_transfer(void)
{
	struct virtio_gpu_transfer_to_host_2d req = {
		.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
		.resource_id = 1,
		.r = {
			.x = 0,
			.y = 0,
			.width = gpu.width,
			.height = gpu.height
		},
		.offset = 0
	};
	struct virtio_gpu_ctrl_hdr resp;

	virtio_gpu_send(&req, sizeof(req), 0, 0, &resp, sizeof(resp));
	virtio_gpu_check_or_die("transer", &resp, 0);
}

void virtio_gpu_flush(void)
{
	struct virtio_gpu_resource_flush req = {
		.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH,
		.resource_id = 1,
		.r = {
			.x = 0,
			.y = 0,
			.width = gpu.width,
			.height = gpu.height
		}
	};
	struct virtio_gpu_ctrl_hdr resp;

	virtio_gpu_send(&req, sizeof(req), 0, 0, &resp, sizeof(resp));
	virtio_gpu_check_or_die("flush", &resp, 0);
}

void virtio_gpu_scanout(void)
{
	struct virtio_gpu_set_scanout req = {
		.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT,
		.scanout_id = 0,
		.resource_id = 1,
		.r = {
			.x = 0,
			.y = 0,
			.width = gpu.width,
			.height = gpu.height
		}
	};
	struct virtio_gpu_ctrl_hdr resp;

	virtio_gpu_send(&req, sizeof(req), 0, 0, &resp, sizeof(resp));
	virtio_gpu_check_or_die("scanout", &resp, 0);
}

void virtio_gpu_send(void *cmd, int cmdlen, void *data, int datalen, void *resp, int resplen)
{
	int cmd_idx = 0; 
	int data_idx = 1;
	int resp_idx = data ? 2 : 1;

	memset(&gpu.desc[cmd_idx], 0, sizeof(gpu.desc[0]));
	gpu.desc[cmd_idx].addr = (uint64)cmd;
	gpu.desc[cmd_idx].len = cmdlen;
	gpu.desc[cmd_idx].flags = VRING_DESC_F_NEXT;
	if (data) gpu.desc[cmd_idx].next = data_idx;
	else gpu.desc[cmd_idx].next = resp_idx;

	if (data)
	{
		memset(&gpu.desc[data_idx], 0, sizeof(gpu.desc[0]));
		gpu.desc[data_idx].addr = (uint64)data;
		gpu.desc[data_idx].len = datalen;
		gpu.desc[data_idx].flags = 0;
		gpu.desc[data_idx].next = resp_idx;
	}

	memset(&gpu.desc[resp_idx], 0, sizeof(gpu.desc[0]));
	gpu.desc[resp_idx].addr = (uint64)resp;
	gpu.desc[resp_idx].len = resplen;
	gpu.desc[resp_idx].flags = VRING_DESC_F_WRITE;

	gpu.avail->ring[gpu.avail->idx % NUM] = cmd_idx;
	__sync_synchronize();
	gpu.avail->idx++;

	*R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

	while (gpu.used->idx != gpu.avail->idx) __sync_synchronize();
}

void virtio_gpu_check_or_die(char *funcname, struct virtio_gpu_ctrl_hdr *resp, uint32 check)
{
	uint32 c = check ? check : VIRTIO_GPU_RESP_OK_NODATA;

	if (resp->type != c)
	{
		printf("GPU error at %s! type: %x, flag: %x!\n", funcname, resp->type, resp->flags);
		panic("GPU panic");
	}
}

void write_ready_screen(void)
{
	int x, y;

printf("in\n");
	for (y = 0; y < gpu.height; y++)
		for (x = 0; x < gpu.width; x++)
		{
			uint64 pix = (y * gpu.width + x);
			uint32* pagebegin = (uint32*)(gpu.fb_addr[pix * 4 / PGSIZE]);

			pagebegin[pix] = RGB(0, 255, 255);
		}
printf("out\n");
}
