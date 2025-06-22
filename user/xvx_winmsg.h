#ifndef __WINMSG_H__
#define __WINMSG_H__

#include "xvx_wintypes.h"

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
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: upper 16bits are width, lower 16bits are height
// extra: struct winmsg_register
#define WM_REGISTER	(WM_BASE + 5)
// param0: winident (-1 if failed)
// param1: reserved
#define WM_REGISTERACK	(WM_BASE + 6)

/*
 * structs
 */
struct winmsg_register
{
	int pid;
	int draw_type;
	char title[32];
};

#endif
