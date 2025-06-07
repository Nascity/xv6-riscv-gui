#ifndef __WINCOMP_H__
#define __WINCOMP_H__

struct wincomponent
{
	wincomp_t id;

	// the type of the component
#define FILL	0
#define BUTTON	1
#define ICON	2
#define TEXT	3
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

	struct wincomponent *next;
};

struct fill_component
{
	// the color of the fill
	int color;
};

struct button_component
{
	// the identifier
	char *text;
	int length;
};

struct icon_component
{
	void *bitmap;
	char *text;
	int length;
};

#endif
