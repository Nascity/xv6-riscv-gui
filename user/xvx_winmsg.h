#ifndef __WINMSG_H__
#define __WINMSG_H__

#include "xvx_wintypes.h"

#define MSG_SZ	128

typedef int code_t;

struct winmsg
{
	winident_t ident;
	code_t code;
	int param0;
	int param1;
#define MAX_EXTRA_SIZE	(MSG_SZ - (sizeof(struct winmsg) - 1))
	char extra[1];
};

/*
 * code_t codes
 */
#define WM_BASE		(20000)
// param0: upper 16bits are x, lower 16bits are y coordinates
// param1: reserved - must be zero
#define WM_BUTTONDOWN	(WM_BASE + 0)
#define WM_BUTTONUP	(WM_BASE + 1)



#endif
