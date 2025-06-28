#include "kernel/types.h"
#include "kernel/input_events.h"
#include "user/user.h"
#include "user/xvx_win.h"

int winid;

int register_window(int pid)
{
	char *title = "Developers";
	char buf_msg[128];
	struct winmsg *msg = (struct winmsg*)buf_msg;
	struct winmsg_register *pwr = (struct winmsg_register*)msg->extra;
	int i;

	for (i = 0; i < 128; i++)
		buf_msg[i] = 0x12 + i;

	msg->ident = 0;
	msg->code = WM_REGISTER;
	msg->param0 = MAKEPARAM(200, 200);
	msg->param1 = MAKEPARAM(500, 500);
	
	pwr->pid = getpid();
	pwr->draw_type = DEFAULT_WINDOW;
	for (i = 0; title[i]; i++)
		pwr->title[i] = title[i];
	pwr->title[i] = 0;
	
	printf("child: %x %x %x %x %x %x %s\n",
			msg->ident, msg->code, msg->param0,
			msg->param1, pwr->pid, pwr->draw_type,
			pwr->title);	// DEBUG
	printf("at: %p\n", buf_msg);

	send_msg(pid, buf_msg, 128);

	while (recv_msg(msg, 128, 1) == -1)
	{
		printf("child here!\n");
		if (msg->code != WM_REGISTERACK)
			return -1;
		else if (msg->param0 == -1)
			return -1;
		else
			break;
	}

	winid = msg->param0;
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 1 || argv[0][0] != 'G')
	{
		printf("This program has to be run in XvX shell.\n");
		return -1;
	}
	if (register_window(argv[0][1] - '\x10'))
		printf("Failed to register window.\n");
}
