#ifndef __WINCOMP_H__
#define __WINCOMP_H__

struct wincomponent
{
	// the type of the component
#define FILL	0
#define BUTTON	1
#define BITMAP	2
	int comp_type;

	// position relative to the parent
	int x;
	int y;
	int width;
	int height;

	// the parent
	struct win *parent;

	// the pointer to the actual component
	void *comp;
};

struct fill_component
{
	// the color of the fill
	int color;
};

struct button_component
{
	// the identifier
	wincomp_t id;
};

#endif
