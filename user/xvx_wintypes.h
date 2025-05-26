#ifndef __WINTYPES_H__
#define __WINTYPES_H__

#include "xvx_windefs.h"

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
	int width;
	int height;

	// parent - null if shell - and children
#define MAX_CHILD	50
	struct win *parent;
	struct win *child[MAX_CHILD];
	int num_children;

	// how to draw the window
#define MAXIMIZE_BUTTON		0b00001001
#define MINIMIZE_BUTTON		0b00001010
#define BORDER			0b00000100
#define UPPER_BAR		0b00001000
#define SHELL_TEST		0b00001111	// only for test purpose
	int win_draw_type;

	int rendered;
};

#endif
