#ifndef __WINTYPES_H__
#define __WINTYPES_H__

#define RGB(r, g, b)	((((r) & 0xFF) << 16) + (((g) & 0xFF) << 8) + ((b) & 0xFF))
#define THEME_COLOR	RGB(0x00, 0xD5, 0xFF)

typedef int winident_t;
typedef int wincomp_t;
typedef unsigned int msgnum_t;
typedef unsigned long long param_t;

struct win
{
	// the window identifier
#define NO_WINIDENT	-1
	winident_t id;

#define MAX_TITLE	32
	char title[MAX_TITLE];

	// the owner of the window/windows in pid
	int owner;

	// position relative to parent struct win
	int x;
	int y;
#define MONITOR_WIDTH	1280
#define MONITOR_HEIGHT	800
	int width;
	int height;

	// parent - null if shell - and children
#define MAX_CHILD	50
	struct win *parent;
	struct win *child[MAX_CHILD];
	int num_children;

	// how to draw the window
#define MAXIMIZE_BUTTON		0b00000001
#define MINIMIZE_BUTTON		0b00000010
#define BORDER			0b00000100
	int win_draw_type;
};

#endif
