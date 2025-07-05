#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "user/xvx_win.h"

#define PADDING		10

#define TIMEOUT		0

int wmpid;
int winid;
int img_width;
int img_height;
int act_width;
int act_height;

int get_image_dimension(char *filename)
{
	int fd;
	char buffer[0x16 + 4];
	int *pwidth, *pheight;

	if ((fd = open(filename, 0)) < 0)
		return -1;

	read(fd, buffer, sizeof(buffer) / sizeof(char));

	pwidth = (int*)&buffer[0x12];
	pheight = (int*)&buffer[0x16];

	act_width = img_width = *pwidth;
	act_height = img_height = *pheight;

	if (img_width < 300)
		img_width = 300;
	if (img_height < 300)
		img_height = 300;

	close(fd);
	return 0;
}

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

int register_window(char *filename)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_register *pwr = (struct winmsg_register*)msg->extra;
	int i;

	msg->ident = 0;
	msg->code = WM_REGISTER;
	msg->param0 = MAKEPARAM((filename[0] % 10 * 100 + 10) % 1280, (filename[1] % 10 * 100 + 10) % 800);
	msg->param1 = MAKEPARAM(img_width + PADDING * 2, img_height + PADDING * 2);
	
	pwr->pid = getpid();
	pwr->draw_type = DEFAULT_WINDOW;
	for (i = 0; filename[i]; i++)
		pwr->title[i] = filename[i];
	pwr->title[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;

	winid = msg->param0;
	return 0;
}

int init_bitmap(char *filename)
{
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_regcomp *pwr = (struct winmsg_regcomp*)msg->extra;
	int i;

	msg->ident = winid;
	msg->code = WM_REGCOMP;
	msg->param0 = MAKEPARAM(PADDING, PADDING);
	msg->param1 = MAKEPARAM(act_width, act_height);

	pwr->pid = getpid();
	pwr->comp_type = BITMAP;
	for (i = 0; filename[i]; i++)
		pwr->u.text.text[i] = filename[i];
	pwr->u.text.text[i] = 0;

	send_msg(wmpid, buf_msg, 128);
	if (wait_ack(msg))
		return -1;
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2 || argv[0][0] != 'G')
	{
		printf("This program has to be run in XvX shell.\n");
		exit(-1);
	}

	wmpid = argv[0][1] - '\x10';

	printf("here0\n");

	if (get_image_dimension(argv[1]))
		printf("Failed to retrieve dimension.\n");
	printf("here1\n");
	if (register_window(argv[1]))
		printf("Failed to register window.\n");
	else if (init_bitmap(argv[1]))
		printf("Failed to register bitmap.\n");
	printf("here2\n");

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

	exit(0);
}
