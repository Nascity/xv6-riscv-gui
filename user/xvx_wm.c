#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "xvx_wintypes.h"
#include "xvx_wincomp.h"

#define MAX_WINDOWS 512
struct win windows[MAX_WINDOWS];

winident_t id_track;
int shell_index;

// event operations
#define TIMEOUT		10
int recv_kernel_msg(struct wmmsg* pmsg);

// window operations
#define BACKGROUND_COLOR	RGB(0, 100, 255)
void init_wm(void);
void exit_wm(int);

int register_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type);

// draw operation
void draw_rect(int x, int y, int width, int height, int rgb);

void draw_cursor(int x, int y, int color);
void update_cursor(int x, int y);
void click_cursor(int x, int y, int pressed);

void invalidate_region(int x, int y, int width, int height);

int screen_buffer[MONITOR_HEIGHT][MONITOR_WIDTH];

// cursor
#define CURSOR_BASE_WIDTH	5
#define CURSOR_BASE_HEIGHT	7
#define CURSOR_RESIZE	5
#define CURSOR_WIDTH	(CURSOR_BASE_WIDTH * CURSOR_RESIZE)
#define CURSOR_HEIGHT	(CURSOR_BASE_HEIGHT * CURSOR_RESIZE)
char *cursor[] = {
	"o    ",
	"oo   ",
	"oxo  ",
	"oxxo ",
	"ooooo",
	" o   ",
	" o   ",
};
int last_pos_x;
int last_pos_y;



int main(int argc, char *argv[])
{
	register_wm();
	init_wm();

	shell_index = register_window("XvX shell", 0, 0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, 0, BORDER);
	if (shell_index == -1)
	{
		printf("Cannot register shell window.\n");
		exit_wm(-1);
	}

	while (1)
	{
		struct wmmsg msg;

		if (recv_kernel_msg(&msg) == -1)
			continue;

		switch (msg.event_code)
		{
		case EV_REL:
			update_cursor(X(msg.param0), Y(msg.param0));
			break;
		case EV_KEY:
			click_cursor(X(msg.param0), Y(msg.param0), msg.param1);
			break;
		}
	}
}

void init_wm(void)
{
	int i, j;

	for (i = 0; i < MAX_WINDOWS; i++)
		windows[i].id = NO_WINIDENT;
	for (i = 0; i < MONITOR_HEIGHT; i++)
		for (j = 0; j < MONITOR_WIDTH; j++)
			screen_buffer[i][j] = BACKGROUND_COLOR;
	draw_fill(0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, BACKGROUND_COLOR);
}

void exit_wm(int exit_code)
{
	unregister_wm();
	exit(exit_code);
}

void draw_rect(int x, int y, int width, int height, int rgb)
{
	int i, j;

	for (i = y; i < height && i < MONITOR_HEIGHT; i++)
		for (j = x; j < width && j < MONITOR_WIDTH; j++)
			if (i > 0 && j > 0)
				screen_buffer[i][j] = rgb;
}

void draw_cursor(int x, int y, int color)
{
	int i, j;
	int cursor_buffer[CURSOR_WIDTH * CURSOR_HEIGHT];

	invalidate_region(last_pos_x, last_pos_y, CURSOR_WIDTH, CURSOR_HEIGHT);
	for (i = 0; i < CURSOR_HEIGHT; i++)
		for (j = 0; j < CURSOR_WIDTH; j++)
		{
			unsigned int col;
			char val = cursor[i / CURSOR_RESIZE][j / CURSOR_RESIZE];

			if (val == ' ')
				col = screen_buffer[y + i][x + j];
			else if (val == 'o')
				col = color;
			else if (val == 'x')
				col = RGB(255, 255, 255);
			else
				col = RGB(255, 0, 0);

			cursor_buffer[i * CURSOR_WIDTH + j] = col;
		}

	draw_bits(x, y, CURSOR_WIDTH, CURSOR_HEIGHT, (uint32*)cursor_buffer, CURSOR_WIDTH * CURSOR_HEIGHT);

	last_pos_x = x;
	last_pos_y = y;
}

void update_cursor(int x, int y)
{
	draw_cursor(x, y, RGB(0, 0, 0));
}

void click_cursor(int x, int y, int pressed)
{
	if (pressed)
		draw_cursor(x, y, RGB(0, 255, 0));
	else
		draw_cursor(x, y, RGB(0, 0, 0));
}

void invalidate_region(int x, int y, int width, int height)
{
	int *buf;
	int i, j;

	if (x < 0)
	{
		width += x;
		x = 0;
	}
	if (y < 0)
	{
		height += y;
		y = 0;
	}

 	buf = malloc(width * height * sizeof(int));
	for (i = 0; i < height && y + i < MONITOR_HEIGHT; i++)
		for (j = 0; j < width && x + j < MONITOR_WIDTH; j++)
			buf[i * width + j] = screen_buffer[i + y][j + x];

	draw_bits(x, y, width, height, (uint32*)buf, width * height);

	free(buf);
}

// receives kernel message and returns it via pointer
// returns -1 if failed
int recv_kernel_msg(struct wmmsg* pmsg)
{
	if (recv_msg(pmsg, sizeof(struct wmmsg), TIMEOUT) == -1)
	{
		printf("DEBUG: timeout\n");
		return -1;
	}
	return 0;
}

// registers a new window in windows array
// returns the index of the array when success
// returns -1 when failed
int register_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type)
{
	int i;
	struct win *ptr;

	for (i = 0; i < MAX_WINDOWS; i++)
		if (windows[i].id == NO_WINIDENT)
			break;
	if (i == MAX_WINDOWS)
		return -1;
	ptr = &windows[i];

	ptr->id = id_track++;
	ptr->owner = owner;
	ptr->x = x;
	ptr->y = y;
	ptr->width = width;
	ptr->height = height;
	ptr->parent = &windows[parent];
	ptr->num_children = 0;
	strcpy(ptr->title, title);

	return i;
}
