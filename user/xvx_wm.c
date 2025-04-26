#include "kernel/types.h"
#include "user/user.h"
#include "wintypes.h"

#define MAX_WINDOWS 512
struct win windows[MAX_WINDOWS];

winident_t id_track;
int shell_index;

// window operations
int register_new_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type);

int main(int argc, char *argv[])
{
	register_wm();

	shell_index = register_new_window("XvX shell", 0, 0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, 0, BORDER);
	if (shell_index == -1)
	{
		printf("Cannot register shell window.\n");
		exit(-1);
	}

	
}

// registers a new window in windows array
// returns the index of the array when success
// returns -1 when failed
int register_new_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type)
{
	int i;
	struct win *ptr;

	for (i = 0; i < MAX_WINDOWS; i++)
		if (windows[i].id == NO_WINIDENT)
			break;
	if (i == MAX_WINDOWS)
		return -1;
	ptr = &windows[i];

	ptr->id = id_track++;
	ptr->owner = owner;
	ptr->x = x;
	ptr->y = y;
	ptr->width = width;
	ptr->height = height;
	ptr->parent = &windows[parent];
	ptr->num_children = 0;
	strcpy(ptr->title, title);

	return i;
}
