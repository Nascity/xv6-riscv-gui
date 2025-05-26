#ifndef __WINDEFS_H__
#define __WINDEFS_H__

#define MONITOR_WIDTH	1280
#define MONITOR_HEIGHT	800

#define RGB(r, g, b)	((((r) & 0xFF) << 16) + (((g) & 0xFF) << 8) + ((b) & 0xFF))
#define GRAY(g)		(RGB((g), (g), (g)))

#define BACKGROUND_COLOR	RGB(0, 100, 255)
#define THEME_COLOR		RGB(0x00, 0xD5, 0xFF)

#define WINDOW_BORDER_COLOR	GRAY(180)
#define WINDOW_BACKGROUND_COLOR	GRAY(200)

#define BORDER_THICKNESS	10

#endif
