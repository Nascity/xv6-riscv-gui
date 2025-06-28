#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "user/xvx_win.h"

#define MONITOR_WIDTH	1280
#define MONITOR_HEIGHT	800

#define WINDOW_WIDTH	1000
#define WINDOW_HEIGHT	500

#define WINDOW_X	((MONITOR_WIDTH - WINDOW_WIDTH) / 2)
#define WINDOW_Y	((MONITOR_HEIGHT - WINDOW_HEIGHT) / 2)

int winid;
int wmpid;

int wait_ack(struct winmsg *msg)
{
	while (recv_msg(msg, 128, -1))
	{
		printf("child here!\n");
		if (msg->code != WM_REGISTERACK)
			return -1;
		else if (msg->param0 == -1)
			return -1;
		else
			break;
	}
	return 0;
}

int register_window()
{
	char *title = "Devs";
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_register *pwr = (struct winmsg_register*)msg->extra;
	int i;

	msg->ident = 0;
	msg->code = WM_REGISTER;
	msg->param0 = MAKEPARAM(WINDOW_X, WINDOW_Y);
	msg->param1 = MAKEPARAM(WINDOW_WIDTH, WINDOW_HEIGHT);
	
	pwr->pid = getpid();
	pwr->draw_type = DEFAULT_WINDOW;
	for (i = 0; title[i]; i++)
		pwr->title[i] = title[i];
	pwr->title[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;

	winid = msg->param0;
	return 0;
}

int init(void)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_regcomp *pwr = (struct winmsg_regcomp*)msg->extra;
	char *path = "/img.bmp";
	int i;

	// register bitmaps
	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM(10, 10);
	msg->param1 = MAKEPARAM(50, 40);

	pwr->pid = getpid();
	pwr->comp_type = BITMAP;
	for (i = 0; path[i]; i++)
		pwr->u.text.text[i] = path[i];
	pwr->u.text.text[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;
	// DO SOMETHING	DEBUG!!
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 1 || argv[0][0] != 'G')
	{
		printf("This program has to be run in XvX shell.\n");
		return -1;
	}

	wmpid = argv[0][1] - '\x10';
	if (register_window())
		printf("Failed to register window.\n");
	else if (init())
		printf("Failed to register components.\n");

	// message loop
	while (1)
	{
		char buf_msg[MSG_SZ];
		struct winmsg *msg = (struct winmsg*)buf_msg;

		if (recv_msg(msg, 128, 10) == -1)
			continue;

		switch (msg->code)
		{
		
		}
	}

	exit(0);
}
