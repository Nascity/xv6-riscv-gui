#ifndef __WINTYPES_H__
#define __WINTYPES_H__

#define MAX_CHILD	50

typedef int winident_t;
typedef unsigned int msgnum_t;
typedef unsigned long long param_t;

struct win
{
	// the window identifier
	winident_t id;

	// the owner of the window/windows
	int owner;

	// position relative to parent struct win
	int x;
	int y;

	// parent - null if shell - and children
	struct win *parent;
	struct win *child[MAX_CHILD];
	int num_children;

	// event handler
	int (*evhandler)(winident_t, msgnum_t, param_t, param_t);
};

#endif
