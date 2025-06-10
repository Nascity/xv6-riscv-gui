#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "xvx_wintypes.h"
#include "xvx_wincomp.h"

struct z_list zl;
winident_t id_track;
int shell_index;

// Z operation
void add_to_top(struct win *pw);
void move_to_top(struct win *pw);

// msg operations
#define TIMEOUT		10
int recv_kernel_msg(struct wmmsg* pmsg);
void send_msg_to_proc(int pid, int code, int param0, int param1);

// window operations
void init_wm(void);
void exit_wm(int);

int register_window(const char *title, int owner, int x, int y, int width, int height, struct win *parent, int draw_type);

// draw operation
void rect(int x, int y, int width, int height, int rgb);

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
#define ALPHABET_BASE_WIDTH		5
#define ALPHABET_BASE_HEIGHT		5
#define GET_ALPHA_FROM_LOWER(ch)	((char*)&alphabet[((ch) - 'a') * ALPHA_BASE_HEIGHT])
#define GET_ALPHA_FROM_UPPER(ch)	((char*)&alphabet[((ch) - 'A') * ALPHA_BASE_HEIGHT])
#define GET_ALPHA(ch)			(((ch) >= 'A') && (ch) <= 'Z'	?	\
					GET_ALPHA_FROM_UPPER(ch)	:	\
					((ch) >= 'a' && (ch) <= 'z'	?	\
					GET_ALPHA_FROM_LOWER(ch)	:	\
					(char*)0))
char *alphabet[] = {
    " ooo ",
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

    "oooo ",
    "o   o",
    "o   o",
    "o   o",
    "oooo ",

    "ooooo",
    "o    ",
    "oooo ",
    "o    ",
    "ooooo",

    "ooooo",
    "o    ",
    "oooo ",
    "o    ",
    "o    ",

    " oooo",
    "o    ",
    "o ooo",
    "o   o",
    " oooo",

    "o   o",
    "o   o",
    "ooooo",
    "o   o",
    "o   o",

    " ooo ",
    "  o  ",
    "  o  ",
    "  o  ",
    " ooo ",

    "    o",
    "    o",
    "    o",
    "o   o",
    " ooo ",

    "o   o",
    "o  o ",
    "ooo  ",
    "o  o ",
    "o   o",

    "o    ",
    "o    ",
    "o    ",
    "o    ",
    "ooooo",

    "o   o",
    "oo oo",
    "o o o",
    "o   o",
    "o   o",

    "o   o",
    "oo  o",
    "o o o",
    "o  oo",
    "o   o",

    " ooo ",
    "o   o",
    "o   o",
    "o   o",
    " ooo ",

    "oooo ",
    "o   o",
    "oooo ",
    "o    ",
    "o    ",

    " ooo ",
    "o   o",
    "o   o",
    "o  o ",
    " oo o",

    "oooo ",
    "o   o",
    "oooo ",
    "o  o ",
    "o   o",

    " oooo",
    "o    ",
    " ooo ",
    "    o",
    "oooo ",

    "ooooo",
    "  o  ",
    "  o  ",
    "  o  ",
    "  o  ",

    "o   o",
    "o   o",
    "o   o",
    "o   o",
    " oooo",

    "o   o",
    "o   o",
    "o   o",
    " o o ",
    "  o  ",

    "o   o",
    "o   o",
    "o o o",
    "oo oo",
    "o   o",

    "o   o",
    " o o ",
    "  o  ",
    " o o ",
    "o   o",

    "o   o",
    "o   o",
    " oooo",
    "    o",
    "oooo ",

    "ooooo",
    "   o ",
    "  o  ",
    " o   ",
    "ooooo"
};



int main(int argc, char *argv[])
{
	register_wm();
	init_wm();

	/*
	shell_index = register_window("XvX shell", 0, 0, 0,
			MONITOR_WIDTH, MONITOR_HEIGHT, 0, SHELL_TEST);
			*/
	shell_index = register_window("XvX shell", 0, 0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, 0, NO_TOP_BAR);
	register_window("Test window", 0, 390, 250, 500, 300, 0, DEFAULT_WINDOW);
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
			render();
			click_cursor(X(msg.param0), Y(msg.param0), msg.param1);
			break;
		}
	}
}

void init_wm(void)
{
	int i, j;

	for (i = 0; i < MONITOR_HEIGHT; i++)
		for (j = 0; j < MONITOR_WIDTH; j++)
			screen_buffer[i][j] = BACKGROUND_COLOR;
	rect(0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, BACKGROUND_COLOR);
}

void exit_wm(int exit_code)
{
	unregister_wm();
	exit(exit_code);
}

void rect(int x, int y, int width, int height, int rgb)
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

	if (x < SAFE_MARGIN || x + CURSOR_WIDTH > MONITOR_WIDTH - SAFE_MARGIN
		|| y < SAFE_MARGIN || y + CURSOR_HEIGHT > MONITOR_HEIGHT - SAFE_MARGIN)
		return;
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
	unsigned int *buf;
	int i, j;

	printf("%d %d %d %d\n", x, y, width, height);

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
	if (x + width > MONITOR_WIDTH)
		width -= x + width - MONITOR_WIDTH;
	if (y + height > MONITOR_HEIGHT)
		height -= y + height - MONITOR_HEIGHT;

	buf = malloc(sizeof(int) * width * height);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			buf[i * width + j] = screen_buffer[i][j];
	draw_bits(x, y, width, height, buf, width * height);

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

void send_msg_to_proc(int pid, int code, int param0, int param1)
{
	struct wmmsg msg;

	msg.event_code = code;
	msg.param0 = param0;
	msg.param1 = param1;

	send_msg(pid, &msg, sizeof(msg));
}

// registers a new window in windows array
// returns the index of the array when success
// returns -1 when failed
int register_window(const char *title, int owner, int x, int y, int width, int height, struct win *parent, int draw_type)
{
	struct win *ptr;

	ptr = malloc(sizeof(struct win));

	ptr->id = id_track++;
	ptr->owner = owner;
	ptr->x = x;
	ptr->y = y;
	ptr->width = width;
	ptr->height = height;
	ptr->parent = parent;
	ptr->num_children = 0;
	ptr->win_draw_type = draw_type;
	ptr->maximized = 0;
	ptr->minimized = 0;
	strcpy(ptr->title, title);

	add_to_top(ptr);

	return ptr->id;
}

void add_to_top(struct win *pw)
{
	struct z *newz = malloc(sizeof(struct z));

	newz->higher = 0;
	newz->lower = zl.top;
	newz->win = pw;
	if (!zl.top && !zl.bottom)
	{
		newz->level = 0;
		zl.bottom = zl.top = newz;
	}
	else
	{
		newz->level = zl.top->level + 1;
		zl.top = newz;
	}
}

void move_to_top(struct win *pw)
{
	struct z *movz;

	for (movz = zl.bottom; movz; movz = movz->higher)
		if (movz->win == pw)
			break;
	if (!movz)
		return;

	if (movz == zl.bottom)
		zl.bottom = zl.bottom->higher;

	movz->lower->higher = movz->higher;
	movz->higher->lower = movz->lower;

	zl.top->higher = movz;
	movz->lower = zl.top;
	movz->level = zl.top->level + 1;

	zl.top = movz;
}

void update_invalid_rect(struct win *pw, struct rect *rct)
{
	if (pw->maximized)
	{
		rct->left = rct-> top = 0;
		rct->right = MONITOR_WIDTH;
		rct->bottom = MONITOR_HEIGHT;
	}
	if (rct->left == rct->right && rct->right == rct->top
		&& rct->top == rct-> bottom && rct->bottom == INVALID_RECT)
	{
		rct->left = pw->x;
		rct->right = pw->x + pw->width;
		rct->top = pw->y;
		rct->bottom = pw->y + pw->height;
		return;
	}
	
	if (pw->x < rct->left)
		rct->left = pw->x;
	if (pw->x + pw->width > rct->right)
		rct->right = pw->x + pw->width;
	if (pw->y < rct->top)
		rct->top = pw->y;
	if (pw->y + pw->height > rct->bottom)
		rct->bottom = pw->y + pw->height;
}

void clear_screen_buffer(void)
{
	int i, j;

	for (i = 0; i < MONITOR_HEIGHT; i++)
		for (j = 0; j < MONITOR_WIDTH; j++)
			screen_buffer[i][j] = RGB(0, 255, 0);
}

void render_top_bar(struct win *pw)
{
	int x, y, width, draw_type;

	if (pw->maximized)
	{
		x = y = 0;
		width = MONITOR_WIDTH;
	}
	else
	{
		x = pw->x;
		y = pw->y;
		width = pw->width;
	}
	draw_type = pw->win_draw_type;

	// bar
	rect(x, y, width, TOP_BAR_HEIGHT, THEME_COLOR);
	// exit
	rect(x + width - TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
			y + TOP_BAR_BUTTON_MARGIN,
			TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
			TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
			EXIT_BUTTON_COLOR);
	// maximize
	if (draw_type & MAXIMIZE_BUTTON)
		rect(x + width - 2 * TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
				y + TOP_BAR_BUTTON_MARGIN,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				MAX_BUTTON_COLOR);
	// minimize
	if (draw_type & MINIMIZE_BUTTON)
		rect(x + width - 3 * TOP_BAR_BUTTON_SIZE + TOP_BAR_BUTTON_MARGIN,
				y + TOP_BAR_BUTTON_MARGIN,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				TOP_BAR_BUTTON_SIZE - TOP_BAR_BUTTON_MARGIN * 2,
				MIN_BUTTON_COLOR);
}

void render_window(struct win *pw)
{
	int x, y, width, height;

	if (pw->maximized)
	{
		x = y = 0;
		width = MONITOR_WIDTH;
		height = MONITOR_HEIGHT;
	}
	else
	{
		x = pw->x;
		y = pw->y;
		width = pw->width;
		height = pw->height;
	}

	if (pw->win_draw_type & BORDER)
	{
		rect(x, y, width, height, WINDOW_BORDER_COLOR);
		rect(x + BORDER_THICKNESS, y + BORDER_THICKNESS,
			width - 2 * BORDER_THICKNESS,
			height - 2 * BORDER_THICKNESS,
			WINDOW_BACKGROUND_COLOR);
	}
	else
		rect(x, y, width, height, WINDOW_BACKGROUND_COLOR);

	if (!(pw->win_draw_type & NO_TOP_BAR))
		render_top_bar(pw);
}

void render_components(struct win *pw)
{

}

void render(void)
{
	struct z *pz;
	struct rect inv;

	inv.left = inv.right = inv.top = inv.bottom = INVALID_RECT;
	clear_screen_buffer();

	for (pz = zl.bottom; pz; pz = pz->higher)
	{
		update_invalid_rect(pz->win, &inv);

		if (pz->win->minimized)
			continue;
		render_window(pz->win);
		render_components(pz->win);
	}

	invalidate_region(inv.left, inv.top, inv.right - inv.left, inv.bottom - inv.top);
}
