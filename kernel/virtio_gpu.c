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

volatile int gpu_panicked = 0;

static void virtio_gpu_fetch_display_info(void);
static void virtio_gpu_create_resource(void);
static void virtio_gpu_attach_memory(void);
static void virtio_gpu_transfer(void);
static void virtio_gpu_flush(void);
static void virtio_gpu_scanout(void);

static void virtio_gpu_apply(void);
static void virtio_gpu_send(void*, int, void*, int, void*, int);
static void virtio_gpu_check_or_die(char*, struct virtio_gpu_ctrl_hdr*, uint32);

static void write_ready_screen(void);

void draw_fill(uint16, uint16, uint16, uint16, uint32);
void draw_bits(uint16, uint16, uint16, uint16, uint32*, int);
void gpu_panic(char*);

static struct gpu
{
	struct virtq_desc *desc;
	struct virtq_avail *avail;
	struct virtq_used *used;

	// lock for gpu operations
	struct spinlock gpu_lock;

	// the dimension of the screen
	uint32 width;
	uint32 height;

	// the physical address of the frame buffer
	volatile uint32* fb_addr[REQUIRED_PAGE_COUNT];

	// req and resp that is needed for gpu opperations
	uint64 alloced_base;
	struct virtio_gpu_transfer_to_host_2d* transfer_buf;
	struct virtio_gpu_resource_flush* flush_buf;
	struct virtio_gpu_ctrl_hdr* resp_buf;
} gpu;

void
virtio_gpu_init(void)
{
	uint32 magic = *R(VIRTIO_MMIO_MAGIC_VALUE);
	uint32 device = *R(VIRTIO_MMIO_DEVICE_ID);
	uint32 vendor = *R(VIRTIO_MMIO_VENDOR_ID);

	initlock(&gpu.gpu_lock, "gpu");

	if (magic != 0x74726976 || vendor != 0x554d4551 || device != 16)
	{
		printf("%x %x %x\n", magic, vendor, device);
		panic("could not find virtio gpu");
	}
	
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

	gpu.alloced_base = (uint64)kalloc();
	gpu.transfer_buf = (struct virtio_gpu_transfer_to_host_2d*)gpu.alloced_base;
	gpu.flush_buf = (struct virtio_gpu_resource_flush*)(gpu.alloced_base + 512);
	gpu.resp_buf = (struct virtio_gpu_ctrl_hdr*)(gpu.alloced_base + 1024);

	virtio_gpu_fetch_display_info();
	virtio_gpu_create_resource();
	virtio_gpu_attach_memory();
	virtio_gpu_scanout();

	printf("GPU initialized: %dx%d.\n", gpu.width, gpu.height);

	write_ready_screen();
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
		.nr_entries = REQUIRED_PAGE_COUNT,
	};
	struct virtio_gpu_ctrl_hdr resp;

	for (int i = 0; i < REQUIRED_PAGE_COUNT; i++)
	{
		uint64 fb = (uint64)kalloc();

		if (!fb)
			panic("not enough memory for GPU!");
		entries[i].addr = fb;
		entries[i].length = PGSIZE;
		entries[i].padding = 0;

		gpu.fb_addr[i] = (volatile uint32*)fb;
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

	memset(gpu.flush_buf, 0, sizeof(req));
	memmove(gpu.transfer_buf, &req, sizeof(req));
	virtio_gpu_send(gpu.transfer_buf, sizeof(struct virtio_gpu_transfer_to_host_2d),
			0, 0, gpu.resp_buf, sizeof(struct virtio_gpu_ctrl_hdr));
	virtio_gpu_check_or_die("transer", gpu.resp_buf, 0);
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

	memset(gpu.flush_buf, 0, sizeof(req));
	memmove(gpu.flush_buf, &req, sizeof(req));
	virtio_gpu_send(gpu.flush_buf, sizeof (struct virtio_gpu_resource_flush),
			0, 0, gpu.resp_buf, sizeof(struct virtio_gpu_ctrl_hdr));
	virtio_gpu_check_or_die("flush", gpu.resp_buf, 0);
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

	acquire(&gpu.gpu_lock);

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

	release(&gpu.gpu_lock);
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

void virtio_gpu_apply(void)
{
	virtio_gpu_transfer();
	virtio_gpu_flush();
}

void write_ready_screen(void)
{
	for (int i = 0; i < gpu.height; i++)
		for (int j = 0; j < gpu.width; j++)
			gpu.fb_addr[PAGE(j, i, gpu)][COORD(j, i, gpu)] = RGB(0, j % 256, i % 256);

	virtio_gpu_apply();
}

static volatile uint32 *get_pixel_addr(uint16 x, uint16 y, int i, int j)
{
	return &gpu.fb_addr[PAGE(x + j, y + i, gpu)][COORD(x + j, y + i, gpu)];
}

void draw_fill(uint16 x, uint16 y, uint16 width, uint16 height, uint32 color)
{
	if (!gpu.fb_addr)
		panic("fb is null!");
	else if (gpu_panicked)
		for (;;);

	for (int i = 0; i < height && y + i < gpu.height; i++)
		for (int j = 0; j < width && x + j < gpu.width; j++)
			*get_pixel_addr(x, y, i, j) = color;

	virtio_gpu_apply();
}

void draw_bits(uint16 x, uint16 y, uint16 width, uint16 height, uint32 *bits, int size)
{
	if (!gpu.fb_addr)
		panic("fb is null!");
	else if (gpu_panicked)
		for (;;);

	for (int i = 0; i < height && y + i < gpu.height; i++)
		for (int j = 0; j < width && x + j < gpu.width; j++)
			if (i * gpu.width + j <= size)
				*get_pixel_addr(x, y, i, j) = bits[i * gpu.width + j];

	virtio_gpu_apply();
}

// THIS IS TEMPORARY!!
// MUST BE FIXED!!
void draw_cursor(uint32 orig_x, uint32 orig_y, uint32 x, uint32 y)
{
	int i, j;

	for (i = 0; i < 50; i++)
		for (j = 0; j < 50; j++)
		{
			if (i > j && y + i > 0 && x + j > 0 && y + i < gpu.height && x + j < gpu.width)
				*get_pixel_addr(x, y, i, j) = RGB(255, 0, 0);
		}

	virtio_gpu_apply();
}

void gpu_panic(char *msg)
{
	if (!gpu.fb_addr)
		panic(0);
	
	draw_fill(0, 0, gpu.width, gpu.height / 3, RGB(75, 75, 75));
	draw_fill(0, gpu.height / 3, gpu.width, 2 * gpu.height / 3, RGB(50, 50, 50));
	draw_fill(0, 2 * gpu.height / 3, gpu.width, gpu.height, RGB(25, 25, 25));

	gpu_panicked = 1;
	for(;;);
}
