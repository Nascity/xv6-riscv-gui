#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "xvx_wintypes.h"
#include "xvx_wincomp.h"

#define MAX_WINDOWS 512
struct win windows[MAX_WINDOWS];

winident_t id_track;
int shell_index;

// event operations
#define TIMEOUT		10
int recv_kernel_msg(struct wmmsg*);

// window operations
void init_wm(void);
void exit_wm(int);
int register_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type);




int main(int argc, char *argv[])
{
	register_wm();
	init_wm();

	shell_index = register_window("XvX shell", 0, 0, 0, MONITOR_WIDTH, MONITOR_HEIGHT, 0, BORDER);
	if (shell_index == -1)
	{
		printf("Cannot register shell window.\n");
		exit_wm(-1);
	}

	while (1)
	{
		struct wmmsg msg;

		if (recv_kernel_msg(&msg) == -1)
			continue;
	}
}

void init_wm(void)
{
	int i;

	for (i = 0; i < MAX_WINDOWS; i++)
		windows[i].id = NO_WINIDENT;
}

void exit_wm(int exit_code)
{
	unregister_wm();
	exit(exit_code);
}

// receives kernel message and returns it via pointer
// returns -1 if failed
int recv_kernel_msg(struct wmmsg* pmsg)
{
	if (recv_msg(pmsg, sizeof(struct wmmsg), TIMEOUT) == -1)
	{
		printf("DEBUG: timeout\n");
		return -1;
	}
	return 0;
}

// registers a new window in windows array
// returns the index of the array when success
// returns -1 when failed
int register_window(const char *title, int owner, int x, int y, int width, int height, winident_t parent, int draw_type)
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
