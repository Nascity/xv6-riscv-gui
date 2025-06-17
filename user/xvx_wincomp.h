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
	int dummy, dummy2;	// I don't know why but these should
	char text[1];		// be here in order to work
};

struct icon_component
{
#define ICON_EXE	0
#define	ICON_BMP	1
#define ICON_DIR	2
#define ICON_ETC	3
#define ICON_TXT	4
#define ICON_COUNT	5
	int type;
	int (*bitmap)[];
	char text[1];
};

struct text_component
{
	int pt;
	int color;
	char text[1];
};

#endif
