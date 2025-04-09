#include "kernel/types.h"
#include "user/user.h"

#define BUFSZ	16

void child(void)
{
	char buf[BUFSZ];

	if (recv_msg(buf, BUFSZ, -1))
		printf("recv error\n");
	printf("recv msg: %s\n", buf);
	exit(0);
}

int
main(void)
{
	int pid = fork();

	if (pid == 0)
		child();

	char* buf = "Hello, world!";

	printf("sent msg: %s\n", buf);
	if (send_msg(pid, buf, BUFSZ))
		printf("send error\n");

	wait(&pid);

	exit(0);
}
