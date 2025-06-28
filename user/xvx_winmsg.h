#ifndef __WINMSG_H__
#define __WINMSG_H__

#include "xvx_wintypes.h"

#define MAX_TEXT	32

/*
 * code_t codes
 */
#define WM_BASE		(20000)
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: wincomp_t that has been pressed
#define WM_BUTTONUP	(WM_BASE + 0)
#define WM_BUTTONDOWN	(WM_BASE + 1)
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: reserved
#define WM_MINIMIZED	(WM_BASE + 2)
#define WM_MAXIMIZED	(WM_BASE + 3)
// param0: reserved
// param1: reserved
#define WM_CLOSE	(WM_BASE + 4)
// param0: winident (-1 if failed)
// param1: reserved
#define WM_REGISTERACK	(WM_BASE + 5)
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: upper 16bits are width, lower 16bits are height
// extra: struct winmsg_register
#define WM_REGISTER	(WM_BASE + 6)
// param0: wincomp_t (-1 if failed)
// param1: reserved
#define WM_REGCOMPACK	(WM_BASE + 7)
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: upper 16bits are width, lower 16bits are height
// extra: struct winmsg_regcomp
#define WM_REGCOMP	(WM_BASE + 8)

/*
 * structs
 */
struct winmsg_register
{
	int pid;
	int draw_type;
	char title[MAX_TEXT];
};

// I hate I had to write this
struct winmsg_regcomp
{
	int pid;
	int comp_type;
	union
	{
		// FILL:	fill_color
		// BUTTON:	text
		// ICON:	icon_index, text
		// TEXT:	text, pt, color
		// BITMAP:	text
		int fill_color;
		struct
		{
			int icon_index;
			char text[MAX_TEXT];
			int pt;
			int color;
		} text;
	} u;
};

#endif
