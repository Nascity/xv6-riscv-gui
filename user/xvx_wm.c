#include "kernel/types.h"
#include "kernel/input_events.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "xvx_wintypes.h"
#include "xvx_wincomp.h"

struct z_list zl;
winident_t id_track;
wincomp_t comp_id_track;
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

void add_to_top(struct win *pw);
void move_to_top(struct win *pw);

// component operations
void *make_fill_comp(int color);
void *make_button_comp(char *text, int len);
void *make_icon_comp(void *bitmap, char *text, int len);
void *make_text_comp(char *text, int len, int pt, int color);
void add_components(struct win *pw, int comptype, int rel_x, int rel_y, int width, int height, void *info);

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
int last_pos_x;
int last_pos_y;

char *cursor[] = {
	"o    ",
	"oo   ",
	"oxo  ",
	"oxxo ",
	"ooooo",
	" o   ",
	" o   ",
};

// text
#define ALPHABET_BASE_SIZE		5
#define ALPHABET_BASE_WIDTH		ALPHABET_BASE_SIZE
#define ALPHABET_BASE_HEIGHT		ALPHABET_BASE_SIZE
#define RATIO(x, pt)			(((x) * ALPHABET_BASE_SIZE) / (pt))
#define SPACE(pt)			((pt) + (pt) / 10)
void chr(int x, int y, int pt, char ch, int color);
void text(int x, int y, int pt, char *text, int color);
int get_text_width(int pt, char *text);

char *alphabet[26][5] = {
	{
    " ooo ",
    "o   o",
    "ooooo",
    "o   o",
    "o   o",
	}, {
    "oooo ",
    "o   o",
    "oooo ",
    "o   o",
    "oooo ",
	}, {
    " oooo",
    "o    ",
    "o    ",
    "o    ",
    " oooo",
	}, {
    "oooo ",
    "o   o",
    "o   o",
    "o   o",
    "oooo ",
	}, {
    "ooooo",
    "o    ",
    "oooo ",
    "o    ",
    "ooooo",
	}, {
    "ooooo",
    "o    ",
    "oooo ",
    "o    ",
    "o    ",
	}, {
    " oooo",
    "o    ",
    "o ooo",
    "o   o",
    " oooo",
	}, {
    "o   o",
    "o   o",
    "ooooo",
    "o   o",
    "o   o",
	}, {
    " ooo ",
    "  o  ",
    "  o  ",
    "  o  ",
    " ooo ",
	}, {
    "    o",
    "    o",
    "    o",
    "o   o",
    " ooo ",
	}, {
    "o   o",
    "o  o ",
    "ooo  ",
    "o  o ",
    "o   o",
	}, {
    "o    ",
    "o    ",
    "o    ",
    "o    ",
    "ooooo",
	}, {
    "o   o",
    "oo oo",
    "o o o",
    "o   o",
    "o   o",
	}, {
    "o   o",
    "oo  o",
    "o o o",
    "o  oo",
    "o   o",
	}, {
    " ooo ",
    "o   o",
    "o   o",
    "o   o",
    " ooo ",
	}, {
    "oooo ",
    "o   o",
    "oooo ",
    "o    ",
    "o    ",
	}, {
    " ooo ",
    "o   o",
    "o   o",
    "o  o ",
    " oo o",
	}, {
    "oooo ",
    "o   o",
    "oooo ",
    "o  o ",
    "o   o",
	}, {
    " oooo",
    "o    ",
    " ooo ",
    "    o",
    "oooo ",
	}, {
    "ooooo",
    "  o  ",
    "  o  ",
    "  o  ",
    "  o  ",
	}, {
    "o   o",
    "o   o",
    "o   o",
    "o   o",
    " oooo",
	}, {
    "o   o",
    "o   o",
    "o   o",
    " o o ",
    "  o  ",
	}, {
    "o   o",
    "o   o",
    "o o o",
    "oo oo",
    "o   o",
	}, {
    "o   o",
    " o o ",
    "  o  ",
    " o o ",
    "o   o",
	}, {
    "o   o",
    "o   o",
    " oooo",
    "    o",
    "oooo ",
	}, {
    "ooooo",
    "   o ",
    "  o  ",
    " o   ",
    "ooooo"
	}
};



int main(int argc, char *argv[])
{
	register_wm();

	shell_index = register_window("XvX shell", 0, 0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, 0, NO_TOP_BAR);
	// register_window("Test window", 0, 390, 250, 500, 300, 0, DEFAULT_WINDOW);
	
	if (shell_index == -1)
	{
		printf("Cannot register shell window.\n");
		exit_wm(-1);
	}

	init_wm();

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
	int fd = open("/", 0);
	struct dirent de;

	if (fd < 0)
	{
		printf("cannot open root dir!\n");
		exit_wm(-1);
	}

	i = j = 0;
	while (read(fd, &de, sizeof(de)) == sizeof(de))
	{
		if (de.inum == 0)
			continue;
		add_components(zl.bottom->win, ICON,
				j * DESKTOP_ICON_WIDTH,
				i * DESKTOP_ICON_HEIGHT,
				DESKTOP_ICON_WIDTH,
				DESKTOP_ICON_HEIGHT,
				make_icon_comp(0, de.name, strlen(de.name))
			      );
	}
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

char **get_alpha(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		ch -= ('a' - 'A');
	if (ch < 'A' || ch > 'Z')
		return 0;

	return alphabet[ch - 'A'];
}

void chr(int x, int y, int pt, char ch, int color)
{
	char **alpha = get_alpha(ch);
	int i, j;

	if (!alpha)
		return;

	for (i = 0; i < pt && y + i < MONITOR_HEIGHT; i++)
	{
		for (j = 0; j < pt && x + j < MONITOR_WIDTH; j++)
		{
			int here_x = RATIO(j, pt);
			int here_y = RATIO(i, pt);

			if (alpha[here_y][here_x] == 'o')
				screen_buffer[y + i][x + j] = color;
		}
	}
}

void text(int x, int y, int pt, char *text, int color)
{
	int j, count;
	
	if (pt < 5)
		pt = 5;
	for (j = x, count = 0;
		j < MONITOR_WIDTH && text[count];
		j += SPACE(pt), count++)
		chr(j, y, pt, text[count], color);
}

int get_text_width(int pt, char *text)
{
	int j, count;

	for (count = 0, j = 0; text[count]; j += SPACE(pt), count++);
	return j;
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

unsigned int buf[1280 * 800];

void invalidate_region(int x, int y, int width, int height)
{
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
	if (x + width > MONITOR_WIDTH)
		width -= x + width - MONITOR_WIDTH;
	if (y + height > MONITOR_HEIGHT)
		height -= y + height - MONITOR_HEIGHT;


	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			buf[i * width + j] = screen_buffer[y + i][x + j];
	draw_bits(x, y, width, height, buf, width * height);
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
	ptr->first = 0;
	ptr->maximized = 0;
	ptr->minimized = 0;
	strcpy(ptr->title, title);

	add_to_top(ptr);

	return ptr->id;
}

void *make_fill_comp(int color)
{
	struct fill_component *pfc = malloc(sizeof(struct fill_component));

	pfc->color = color;

	return (void*)pfc;
}

void *make_button_comp(char *text, int len)
{
	struct button_component *pbc = malloc(sizeof(struct button_component) + len);
	int i;

	pbc->length = len;
	for (i = 0; i < len; i++)
		pbc->text[i] = text[i];

	return (void*)pbc;
}

void *make_icon_comp(void *bitmap, char *text, int len)
{
	struct icon_component *pic = malloc(sizeof(struct icon_component) + len);
	int i;

	pic->length = len;
	pic->bitmap = bitmap;
	for (i = 0; i < len; i++)
		pic->text[i] = text[i];

	return (void*)pic;
}

void *make_text_comp(char *text, int len, int pt, int color)
{
	struct text_component *ptc = malloc(sizeof(struct text_component) + len);
	int i;

	ptc->length = len;
	ptc->pt = pt;
	ptc->color = color;
	for (i = 0; i < len; i++)
		ptc->text[i] = text[i];

	return (void*)ptc;
}

void add_components(struct win *pw, int comptype, int rel_x, int rel_y, int width, int height, void *info)
{
	struct wincomponent *pwc = malloc(sizeof(struct wincomponent));

	pwc->id = comp_id_track++;
	pwc->comp_type = comptype;
	pwc->x = rel_x;
	pwc->y = rel_y;
	pwc->width = width;
	pwc->height = height;
	pwc->parent = pw;
	pwc->comp = info;
	pwc->next = pw->first;

	pw->first = pwc;
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

	// title
	int text_width = get_text_width(TOP_BAR_TEXT_SIZE, pw->title);
	text(x + (width - text_width) / 2,
		y + TOP_BAR_TEXT_MARGIN,
		TOP_BAR_TEXT_SIZE,
		pw->title,
		RGB(0, 0, 0)
		);
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
	int base_x, base_y;

	base_x = pw->x;
	base_y = pw->y + TOP_BAR_HEIGHT;
	if (pw->win_draw_type & BORDER)
		base_x += BORDER_THICKNESS;

	for (struct wincomponent *ptr = pw->first; ptr; ptr = ptr->next)
	{
		switch (ptr->comp_type)
		{
		case FILL:
			rect(base_x + ptr->x, base_y + ptr->y,
				ptr->width, ptr->height,
				((struct fill_component*)ptr->comp)->color);
			break;
		case BUTTON:
			
			break;
		case ICON:

			break;
		case TEXT:
			text(base_x + ptr->x, base_y + ptr->y,
				((struct text_component*)ptr->comp)->pt,
				((struct text_component*)ptr->comp)->text,
				((struct text_component*)ptr->comp)->color);
			break;
		default:
			break;
		}
	}
}

void render(void)
{
	struct z *pz;

	for (pz = zl.bottom; pz; pz = pz->higher)
	{
		if (pz->win->minimized)
			continue;
		render_window(pz->win);
		render_components(pz->win);
	}

	invalidate_region(0, 0, MONITOR_WIDTH, MONITOR_HEIGHT);
}
