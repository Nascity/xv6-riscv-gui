#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "user/xvx_win.h"

#define MONITOR_WIDTH	1280
#define MONITOR_HEIGHT	800

#define WINDOW_WIDTH	1000
#define WINDOW_HEIGHT	500

#define IMAGE_WIDTH	167
#define IMAGE_HEIGHT	256

#define LOGO_WIDTH	234
#define LOGO_HEIGHT	207

#define PADDING		20
#define PT		20
#define TEXT_PADDING	5

#define BAR_WIDTH	500
#define BAR_HEIGHT	100

#define TIMEOUT		0

#define WINDOW_X	((MONITOR_WIDTH - WINDOW_WIDTH) / 2)
#define WINDOW_Y	((MONITOR_HEIGHT - WINDOW_HEIGHT) / 2)

#define FILL_Y		(PADDING + IMAGE_HEIGHT + TEXT_PADDING + PT + TEXT_PADDING + 2 * PADDING)

int winid;
int wmpid;

int wait_ack(struct winmsg *msg)
{
	while (recv_msg(msg, 128, -1))
	{
		printf("child here!\n");	// DEBUG
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

int init_bitmaps(void)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_regcomp *pwr = (struct winmsg_regcomp*)msg->extra;
	char *path[3] = { "/shin.bmp", "/joo.bmp", "/hwang.bmp" };
	char *logopath = "/logo.bmp";
	int i, j;
	int x;

	for (i = 0, x = PADDING; i < 3; i++, x += IMAGE_WIDTH + PADDING)
	{
		// register bitmaps
		msg->ident = winid;
		msg->code = WM_REGCOMP;
		msg->param0 = MAKEPARAM(x, PADDING);
		msg->param1 = MAKEPARAM(IMAGE_WIDTH, IMAGE_HEIGHT);

		pwr->pid = getpid();
		pwr->comp_type = BITMAP;
		for (j = 0; path[i][j]; j++)
			pwr->u.text.text[j] = path[i][j];
		pwr->u.text.text[j] = 0;

		send_msg(wmpid, buf_msg, 128);
		if (wait_ack(msg))
			return -1;
	}

	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM((WINDOW_WIDTH + x - LOGO_WIDTH) / 2, (FILL_Y - LOGO_HEIGHT) / 2);
	msg->param1 = MAKEPARAM(LOGO_WIDTH, LOGO_HEIGHT);

	pwr->pid = getpid();
	pwr->comp_type = BITMAP;
	for (i = 0; logopath[i]; i++)
		pwr->u.text.text[i] = logopath[i];
	pwr->u.text.text[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;
	return 0;
}

int init_texts(void)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_regcomp *pwr = (struct winmsg_regcomp*)msg->extra;

	char *surname[3] = { "SHIN", "JOO", "HWANG" };
	char *name[3] = { "SEUNGRI", "HYEONWOO", "SOOJIN" };
	char *disp_text = "THANKS FOR HELPING ME OUT";
	char *disp_text2 = "DZIEKUJE ZA POMOC";

	int i, j;
	int x;

	for (i = 0, x = PADDING; i < 3; i++, x += IMAGE_WIDTH + PADDING)
	{
		// surname
		msg->ident = winid;
		msg->code = WM_REGCOMP;
		msg->param0 = MAKEPARAM(x, PADDING + IMAGE_HEIGHT + TEXT_PADDING);
		msg->param1 = 0;

		pwr->pid = getpid();
		pwr->comp_type = TEXT;
		pwr->u.text.pt = PT;
		pwr->u.text.color = GRAY(0);
		for (j = 0; surname[i][j]; j++)
			pwr->u.text.text[j] = surname[i][j];
		pwr->u.text.text[j] = 0;

		send_msg(wmpid, buf_msg, 128);
		if (wait_ack(msg))
			return -1;

		// name
		msg->ident = winid;
		msg->code = WM_REGCOMP;
		msg->param0 = MAKEPARAM(x, PADDING + IMAGE_HEIGHT
				+ TEXT_PADDING + PT + TEXT_PADDING);
		msg->param1 = 0;

		pwr->pid = getpid();
		pwr->comp_type = TEXT;
		pwr->u.text.pt = PT;
		pwr->u.text.color = GRAY(0);
		for (j = 0; name[i][j]; j++)
			pwr->u.text.text[j] = name[i][j];
		pwr->u.text.text[j] = 0;

		send_msg(wmpid, buf_msg, 128);
		if (wait_ack(msg))
			return -1;
	}

	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM(PADDING * 2 - 5, PADDING + IMAGE_HEIGHT + TEXT_PADDING + PT
				+ TEXT_PADDING + 3 * PADDING - 5);
	msg->param1 = 0;

	pwr->pid = getpid();
	pwr->comp_type = TEXT;
	pwr->u.text.pt = PT;
	pwr->u.text.color = GRAY(255);
	for (i = 0; disp_text[i]; i++)
		pwr->u.text.text[i] = disp_text[i];
	pwr->u.text.text[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;

	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM(PADDING * 2 - 5, PADDING + IMAGE_HEIGHT + TEXT_PADDING + PT
				+ TEXT_PADDING + 3 * PADDING + PT + 5);
	msg->param1 = 0;

	pwr->pid = getpid();
	pwr->comp_type = TEXT;
	pwr->u.text.pt = PT;
	pwr->u.text.color = GRAY(255);
	for (i = 0; disp_text2[i]; i++)
		pwr->u.text.text[i] = disp_text2[i];
	pwr->u.text.text[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;

	return 0;
}

int init_textbar(void)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_regcomp *pwr = (struct winmsg_regcomp*)msg->extra;

	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM(PADDING, FILL_Y);
	msg->param1 = MAKEPARAM(WINDOW_WIDTH - 3 * PADDING, BAR_HEIGHT - PADDING);
	
	pwr->pid = getpid();
	pwr->comp_type = FILL;
	pwr->u.fill_color = RGB(255, 0, 0);

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 1 || argv[0][0] != 'G')
	{
		printf("This program has to be run in XvX shell.\n");
		exit(-1);
	}

	wmpid = argv[0][1] - '\x10';
	if (register_window())
		printf("Failed to register window.\n");
	else if (init_texts())
		printf("Failed to register texts.\n");
	else if (init_textbar())
		printf("Failed to register text bar.\n");
	else if (init_bitmaps())
		printf("Failed to register bitmaps.\n");

	else
	{
		// message loop
		while (1)
		{
			char buf_msg[MSG_SZ];
			struct winmsg *msg = (struct winmsg*)buf_msg;

			if (recv_msg(msg, 128, TIMEOUT) == -1)
				continue;

			switch (msg->code)
			{
			case WM_CLOSE:
				exit(0);
				break;
			default:
				break;
			}
		}
	}

	exit(0);
}
