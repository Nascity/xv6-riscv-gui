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
void init_wm(void);
void exit_wm(int);

int register_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type);

// draw operation
void draw_rect(int x, int y, int width, int height, int rgb);

void draw_cursor(int x, int y, int color);
void update_cursor(int x, int y);
void click_cursor(int x, int y, int pressed);

void invalidate_region(int x, int y, int width, int height);
void render(void);

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

// text
char *uppercase[26] = {
	"ooooo",
	"o   o",
	"ooooo",
	"o   o",
	"o   o",

	"oooo ",
	"o   o",
	"oooo ",
	"o   o",
	"oooo ",

	" oooo",
	"o    ",
	"o    ",
	"o    ",
	" oooo",
};



int main(int argc, char *argv[])
{
	register_wm();
	init_wm();

	/*
	shell_index = register_window("XvX shell", 0, 0, 0,
			MONITOR_WIDTH, MONITOR_HEIGHT, 0, SHELL_TEST);
			*/
	shell_index = register_window("XvX shell", 0, 390, 250, 500, 300, 0, SHELL_TEST);
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

		render();
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

	for (i = y; i < y + height && i < MONITOR_HEIGHT; i++)
		for (j = x; j < x + width && j < MONITOR_WIDTH; j++)
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

	if (width * height <= 4096)
	{
		buf = malloc(width * height * sizeof(int));
		for (i = 0; i < height; i++)
			for (j = 0; j < width; j++)
				buf[i * width + j] = screen_buffer[i + y][j + x];
		draw_bits(x, y, width, height, (uint32*)buf, width * height);
		free(buf);
	}
	else
	{
		int grid_x, grid_y;

		grid_x = width % 32 == 0 ? width / 32 : width / 32 + 1;
		grid_y = height % 32 == 0 ? height / 32 : height / 32 + 1;

		for (i = 0; i < grid_y; i++)
			for (j = 0; j < grid_x; j++)
			{
				invalidate_region(x + j * 32, y + i * 32, 32, 32);
			}
	}

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
	ptr->win_draw_type = draw_type;
	ptr->maximized = 0;
	ptr->rendered = 0;
	strcpy(ptr->title, title);

	return i;
}

void render_top_bar(int x, int y, int width, int draw_type)
{
	// bar
	draw_rect(x, y, width, TOP_BAR_HEIGHT, THEME_COLOR);
	// exit
	draw_rect(x + width - TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
			y + TOP_BAR_BUTTON_MARGIN,
			TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
			TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
			EXIT_BUTTON_COLOR);
	// maximize
	if (draw_type & MAXIMIZE_BUTTON)
		draw_rect(x + width - 2 * TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
				y + TOP_BAR_BUTTON_MARGIN,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				MAX_BUTTON_COLOR);
	// minimize
	if (draw_type & MINIMIZE_BUTTON)
		draw_rect(x + width - 3 * TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
				y + TOP_BAR_BUTTON_MARGIN,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				MIN_BUTTON_COLOR);
}

void render(void)
{
	int i;
	struct win *pw;
	int x, y, width, height;

	for (i = 0; i < MAX_WINDOWS; i++)
	{
		pw = &windows[i];
		if (pw->id == NO_WINIDENT || pw->rendered)
			continue;

		// window minized
		if (pw->maximized)
		{
			x = 0;
			y = 0;
			width = MONITOR_WIDTH;
			height = MONITOR_WIDTH;
		}
		else
		{
			x = pw->x;
			y = pw->y;
			width = pw->width;
			height = pw->height;
		}

		// window background and border
		if (pw->win_draw_type & BORDER)
		{
			draw_rect(x, y, width, height, WINDOW_BORDER_COLOR);
			draw_rect(x + BORDER_THICKNESS, y + BORDER_THICKNESS,
				width - 2 * BORDER_THICKNESS, height - 2 * BORDER_THICKNESS,
				WINDOW_BACKGROUND_COLOR);
		}

		// make top bar
		render_top_bar(x, y, width, pw->win_draw_type);

		invalidate_region(x, y, width, height);
		pw->rendered = 1;
	}
}
