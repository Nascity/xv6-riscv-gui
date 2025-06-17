#include "kernel/types.h"
#include "kernel/input_events.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "xvx_wintypes.h"
#include "xvx_wincomp.h"
#include "xvx_winmsg.h"

struct z_list zl;
winident_t id_track;
wincomp_t comp_id_track;
struct win *shell;
struct win *welcome;

// Z operation
void add_to_top(struct win *pw);
void move_to_top(struct win *pw);

// msg operations
#define TIMEOUT		10
int recv_kernel_msg(struct winmsg* pmsg);
void send_msg_to_proc(int pid, winident_t id, int code, int param0, int param1);

// window operations
void init_wm(void);
void init_welcome(void);
void exit_wm(int);

struct win *register_window(const char *title, int owner, int x, int y, int width, int height, struct win *parent, int draw_type);
void destroy_window(struct win *pw);
struct win *find_window_in_coord(int x, int y);
struct wincomponent *find_component_in_coord(struct win *pw, int x, int y);
int check_top_bar_click(struct win *pw, int x, int y);

// component operations
void *make_fill_comp(int color);
void *make_button_comp(char *text, int len);
void *make_icon_comp(int type, char *text, int len);
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

// icon & bitmap
#define ICON_WIDTH	50
#define ICON_HEIGHT	40
#define BMP_HEADER_SIZE	138
void load_bitmaps(void);
void bits_from_2d(int x, int y, int width, int height, int (*bitmap)[]);

int icons[ICON_COUNT][ICON_HEIGHT][ICON_WIDTH];
const char *paths[ICON_COUNT] = {
	"/exe.bmp",
	"/img.bmp",
	"/dir.bmp",
	0
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
	load_bitmaps();

	shell = register_window("XvX shell", getpid(),
			0, 0, MONITOR_WIDTH, MONITOR_HEIGHT,
			0, NO_TOP_BAR);
	welcome = register_window("WELCOME", getpid(),
			WELCOME_WINDOW_X, WELCOME_WINDOW_Y,
			WELCOME_WINDOW_WIDTH, WELCOME_WINDOW_HEIGHT,
			shell, DEFAULT_WINDOW);
	
	if (!shell)
	{
		printf("Cannot register shell window.\n");
		exit_wm(-1);
	}

	init_wm();
	init_welcome();

	while (1)
	{
		struct winmsg msg;

		if (recv_kernel_msg(&msg) == -1)
			continue;

		switch (msg.code)
		{
		case EV_REL:
			update_cursor(X(msg.param0), Y(msg.param0));
			break;
		case EV_KEY:
			click_cursor(X(msg.param0), Y(msg.param0), msg.param1);
			render();
			if (msg.param1)
				draw_cursor(X(msg.param0), Y(msg.param0), RGB(0, 255, 0));
			else
				draw_cursor(X(msg.param0), Y(msg.param0), RGB(0, 0, 0));
			break;
		}
	}
}

void load_bitmaps(void)
{
	int i, x, y;
	int count;
	char buffer[BMP_HEADER_SIZE];
	char color_buf[3];

	for (i = 0; i < ICON_COUNT; i++)
	{
		if (!paths[i])
			continue;

		int fd = open(paths[i], 0);

		if (fd < 0)
		{
			printf("Failed to open %d-th bmp.\n", i);
			exit_wm(-1);
		}

		read(fd, buffer, 0x0D);
		int size = (int)buffer[0x0A];
		read(fd, buffer, size - 0x0D);

		for (y = ICON_HEIGHT - 1, count = 0; y >= 0; y--, count = 0)
		{
			for (x = 0; x < ICON_WIDTH; x++, count += 3)
			{
				// works for some reason
				color_buf[0] = 0;
				color_buf[1] = 0;
				color_buf[2] = 0;

				read(fd, color_buf, 3);
				icons[i][y][x] = RGB(color_buf[2],
						color_buf[1],
						color_buf[0]);
			}
			read(fd, buffer, 4 - (count / 3) % 4);
		}

		close(fd);
	}
}

int is_ext(char *filename, char *extension)
{
	int i;

	for (i = 0; filename[i]; i++)
		if (filename[i] == '.')
			return !strcmp(&filename[i + 1], extension);
	return 0;
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
		if (de.inum == 0 || de.name[0] == '.')
			continue;

		struct stat st;
		int type = ICON_ETC;

		stat(de.name, &st);
		switch (st.type)
		{
		case T_FILE:
			type = ICON_EXE;
			break;
		case T_DIR:
			type = ICON_DIR;
			break;
		}
		if (is_ext(de.name, "bmp"))
			type = ICON_BMP;
		else if (is_ext(de.name, "txt") || is_ext(de.name, "md"))
			type = ICON_ETC;

		add_components(shell, ICON,
			j * (DESKTOP_ICON_WIDTH + DESKTOP_ICON_MARGIN)
				+ DESKTOP_ICON_MARGIN,
			i * (DESKTOP_ICON_HEIGHT + DESKTOP_ICON_MARGIN)
				+ DESKTOP_ICON_MARGIN,
			DESKTOP_ICON_WIDTH,
			DESKTOP_ICON_HEIGHT,
			make_icon_comp(type, de.name, strlen(de.name)));

		i++;
		if (i >= (MONITOR_HEIGHT - DESKTOP_ICON_MARGIN * i) / DESKTOP_ICON_HEIGHT)
		{
			i = 0;
			j++;
		}
	}

	add_components(shell, FILL, 0, 0,
			MONITOR_WIDTH, MONITOR_HEIGHT,
			make_fill_comp(BACKGROUND_COLOR));
}

void init_welcome(void)
{
	int actual_height = welcome->height - TOP_BAR_HEIGHT;
	char *welcome_msg = "Welcome to XvX";

	struct text_component *test;

	add_components(welcome, TEXT,
			(welcome->width - get_text_width(WELCOME_MSG_SIZE, welcome_msg)) / 2,
			(actual_height - WELCOME_MSG_SIZE) / 2 - 50, 0, 0,
			make_text_comp(welcome_msg, strlen(welcome_msg),
				WELCOME_MSG_SIZE, 0));
	add_components(welcome, BUTTON,
			(welcome->width - WELCOME_BUTTON_WIDTH) / 2,
			(welcome->height - WELCOME_BUTTON_HEIGHT) / 2 + 50,
			WELCOME_BUTTON_WIDTH,
			WELCOME_BUTTON_HEIGHT,
			test = make_button_comp("OK", 3));
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

void bits_from_2d(int x, int y, int width, int height, int (*bitmap)[ICON_WIDTH])
{
	int i, j;

	for (i = y; i < y + height && i < MONITOR_HEIGHT; i++)
	{
		for (j = x; j < x + width && j < MONITOR_WIDTH; j++)
		{
			if (i > 0 && j > 0)
				screen_buffer[i][j] = bitmap[i - y][j - x];
		}
	}
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

			if (val == ' ' && y + i < MONITOR_HEIGHT && x + j < MONITOR_WIDTH)
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
	invalidate_region(last_pos_x, last_pos_y, CURSOR_WIDTH, CURSOR_HEIGHT);
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
	struct win *pw = find_window_in_coord(x, y);
	struct wincomponent *pcomp = 0;
	wincomp_t compid;

	// sending the window message
	if (pw)
		pcomp = find_component_in_coord(pw, x, y);
	if (pcomp)
		printf("id: %d\n", pcomp->id);

	if (pcomp)
		compid = pcomp->id;
	else
		compid = 0;

	if (!pressed)
	{
		switch (check_top_bar_click(pw, x, y))
		{
		case 0:
			send_msg_to_proc(pw->owner, pw->id,
				WM_BUTTONUP + pressed,
				MAKEPARAM(x, y), compid);
			break;
		case 1:
			send_msg_to_proc(pw->owner, pw->id,
				WM_MINIMIZED, MAKEPARAM(x, y), 0);
			pw->minimized = 1;
			break;
		case 2:
			send_msg_to_proc(pw->owner, pw->id,
				WM_MAXIMIZED, MAKEPARAM(x, y), 0);
			pw->maximized = 1 - pw->maximized;
			break;
		case 3:
			send_msg_to_proc(pw->owner, pw->id,
				WM_CLOSE, MAKEPARAM(x, y), 0);
			destroy_window(pw);
			break;
		default:
			break;
		}
	}

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
int recv_kernel_msg(struct winmsg* pmsg)
{
	if (recv_msg(pmsg, sizeof(struct winmsg), TIMEOUT) == -1)
		return -1;
	return 0;
}

void send_msg_to_proc(int pid, winident_t id, int code, int param0, int param1)
{
	struct winmsg msg;

	msg.ident = id;
	msg.code = code;
	msg.param0 = param0;
	msg.param1 = param1;

	send_msg(pid, &msg, sizeof(msg));
}

struct win *register_window(const char *title, int owner, int x, int y, int width, int height, struct win *parent, int draw_type)
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

	for (int i = 0; i < MAX_CHILD; i++)
		ptr->child[i] = 0;

	add_to_top(ptr);

	return ptr;
}

void destroy_window(struct win *pw)
{
	// set childs' parent to destorying window's parent
	for (int i = 0; i < MAX_CHILD; i++)
		if (pw->child[i])
			pw->child[i]->parent = pw->parent;

	// destory wincomponents
	for (struct wincomponent *pwc = pw->first, *next; pwc; pwc = next)
	{
		next = pwc->next;
		free(pwc);
	}

	// free from z_list
	struct z *pz;
	for (pz = zl.bottom; pz; pz = pz->higher)
		if (pz->win == pw)
			break;
	if (!pz)
		return;
	if (pz->lower)
		pz->lower->higher = pz->higher;
	if (pz->higher)
		pz->higher->lower = pz->lower;
	free(pz);

	// free the sturct win
	free(pw);
}

int in_rect(int x, int y, int comp_x, int comp_y, int width, int height)
{
	int x_diff, y_diff;

	x_diff = x - comp_x;
	y_diff = y - comp_y;

	if (x_diff >= 0 && y_diff >= 0
		&& x_diff <= width
		&& y_diff <= height)
		return 1;
	return 0;
}

struct win *find_window_in_coord(int x, int y)
{
	struct z *pz;
	int win_x, win_y, win_w, win_h;

	for (pz = zl.top; pz; pz = pz->lower)
	{
		if (pz->win->minimized)
			continue;
		if (pz->win->maximized)
		{
			win_x = win_y = 0;
			win_w = MONITOR_WIDTH;
			win_h = MONITOR_HEIGHT;
		}
		else
		{
			win_x = pz->win->x;
			win_y = pz->win->y;
			win_w = pz->win->width;
			win_h = pz->win->height;
		}

		if (in_rect(x, y, win_x, win_y, win_w, win_h))
			return pz->win;
	}
	
	return 0;
}

struct wincomponent *find_component_in_coord(struct win *pw, int x, int y)
{
	struct wincomponent *pwc, *ret = 0;
	int win_x, win_y;

	if (pw->minimized)
		return 0;
	if (pw->maximized)
		win_x = win_y = 0;
	else
	{
		win_x = pw->x;
		win_y = pw->y;
	}
	if (pw->win_draw_type & BORDER)
		win_x += BORDER_THICKNESS;
	if (!(pw->win_draw_type & NO_TOP_BAR))
		win_y += TOP_BAR_HEIGHT;

	for (pwc = pw->first; pwc; pwc = pwc->next)
		if (in_rect(x - win_x, y - win_y, pwc->x, pwc->y, pwc->width, pwc->height))
			ret = pwc;

	return ret;
}

// returns
// 0: when no click
// 1: when minimized clicked
// 2: when maximized clicked
// 3: when closed clicked
int check_top_bar_click(struct win *pw, int x, int y)
{
	int win_x, win_y, win_w;

	if (pw->minimized || (pw->win_draw_type & NO_TOP_BAR))
		return 0;
	else if (pw->maximized)
	{
		win_x = x;
		win_y = y;
		win_w = MONITOR_WIDTH;
	}
	else
	{
		win_x = x - pw->x;
		win_y = y - pw->y;
		win_w = pw->width;
	}

	if (in_rect(win_x, win_y, EXIT_BUTTON_X_OFFSET(win_w), TOP_BAR_BUTTON_Y_OFFSET,
			TOP_BAR_BUTTON_SIZE, TOP_BAR_BUTTON_SIZE))
		return 3;
	if (in_rect(win_x, win_y, MAX_BUTTON_X_OFFSET(win_w), TOP_BAR_BUTTON_Y_OFFSET,
			TOP_BAR_BUTTON_SIZE, TOP_BAR_BUTTON_SIZE))
		return 2;
	if (in_rect(win_x, win_y, MIN_BUTTON_X_OFFSET(win_w), TOP_BAR_BUTTON_Y_OFFSET,
			TOP_BAR_BUTTON_SIZE, TOP_BAR_BUTTON_SIZE))
		return 1;
	return 0;
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

	for (i = 0; i < len && text[i]; i++)
		pbc->text[i] = text[i];
	pbc->text[i] = 0;

	return (void*)pbc;
}

void *make_icon_comp(int type, char *text, int len)
{
	struct icon_component *pic = malloc(sizeof(struct icon_component) + len);
	int i;

	pic->type = type;
	pic->bitmap = icons[type];
	for (i = 0; i < len && text[i]; i++)
		pic->text[i] = text[i];
	pic->text[i] = 0;

	return (void*)pic;
}

void *make_text_comp(char *text, int len, int pt, int color)
{
	struct text_component *ptc = malloc(sizeof(struct text_component) + len);
	int i;

	ptc->pt = pt;
	ptc->color = color;
	for (i = 0; i < len && text[i]; i++)
		ptc->text[i] = text[i];
	ptc->text[i] = 0;

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
		zl.top->higher = newz;
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

	if (movz == zl.bottom) zl.bottom = zl.bottom->higher;

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
	rect(x + EXIT_BUTTON_X_OFFSET(width),
		y + TOP_BAR_BUTTON_Y_OFFSET,
		TOP_BAR_BUTTON_SIZE,
		TOP_BAR_BUTTON_SIZE,
		EXIT_BUTTON_COLOR);
	// maximize
	if (draw_type & MAXIMIZE_BUTTON)
		rect(x + MAX_BUTTON_X_OFFSET(width),
			y + TOP_BAR_BUTTON_Y_OFFSET,
			TOP_BAR_BUTTON_SIZE,
			TOP_BAR_BUTTON_SIZE,
			MAX_BUTTON_COLOR);
	// minimize
	if (draw_type & MINIMIZE_BUTTON)
		rect(x + MIN_BUTTON_X_OFFSET(width),
			y + TOP_BAR_BUTTON_Y_OFFSET,
			TOP_BAR_BUTTON_SIZE,
			TOP_BAR_BUTTON_SIZE,
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

	rect(x + WINDOW_SHADOW_OFFSET, y + WINDOW_SHADOW_OFFSET, width, height, WINDOW_SHADOW_COLOR);
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
	base_y = pw->y;

	if (!(pw->win_draw_type & NO_TOP_BAR))
		base_y += TOP_BAR_HEIGHT;
	if (pw->win_draw_type & BORDER)
		base_x += BORDER_THICKNESS;

	for (struct wincomponent *ptr = pw->first; ptr; ptr = ptr->next)
	{
		int x, y;
		int pt, pt2;

		x = base_x + ptr->x;
		y = base_y + ptr->y;

		switch (ptr->comp_type)
		{
		case FILL:
			rect(x, y, ptr->width, ptr->height,
				((struct fill_component*)ptr->comp)->color);
			break;
		case BUTTON:
			pt = ptr->height - 2 * COMP_BUTTON_TEXT_MARGIN;
			pt2 = ptr->width - 2 * COMP_BUTTON_TEXT_MARGIN;
			if (pt2 < pt && pt2 >= 5)
				pt = pt2;
			rect(x, y, ptr->width, ptr->height, GRAY(0));
			rect(x + COMP_BUTTON_MARGIN, y + COMP_BUTTON_MARGIN,
				ptr->width - 2 * COMP_BUTTON_MARGIN,
				ptr->height - 2 * COMP_BUTTON_MARGIN,
				COMP_BUTTON_COLOR);

			text(x + (ptr->width
					- get_text_width(pt,
					((struct text_component*)ptr->comp)->text)
					) / 2,
				y + (ptr->height - pt) / 2,
				pt,
				((struct text_component*)ptr->comp)->text,
				COMP_BUTTON_OUTLINE_COLOR);
			break;
		case ICON:
			pt = ICON_TEXT_SIZE;
			bits_from_2d(x, y, ICON_WIDTH, ICON_HEIGHT,
				((struct icon_component*)ptr->comp)->bitmap);
			text(x + (ICON_WIDTH
					- get_text_width(pt,
					((struct icon_component*)ptr->comp)->text)
					 ) / 2,
				y + 45,
				pt,
				((struct icon_component*)ptr->comp)->text,
				GRAY(0));
			break;
		case TEXT:
			text(x, y, ((struct text_component*)ptr->comp)->pt,
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
