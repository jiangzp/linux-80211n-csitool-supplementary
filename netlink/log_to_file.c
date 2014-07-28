
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define MAX_PAYLOAD 2048
#define SLOW_MSG_CNT 50

int sock_fd = -1;							// the socket
FILE* out = NULL;

FILE* open_file(char* filename, char* spec);

void caught_signal(int sig);

void exit_program(int code);
void exit_program_err(int code, char* func);

int main(int argc, char** argv)
{
	/* Local variables */
	struct sockaddr_nl proc_addr, kern_addr;	// addrs for recv, send, bind
	struct cn_msg *cmsg;
	char buf[99999];
	int ret;
	unsigned short l;
	unsigned short  l2;
	int count = 0;
        uint32_t display_interval;


	/* Open and check log file */
	out = open_file(argv[1], "w");

        if (argc >= 3 || (1 != sscanf(argv[2],"%u",&display_interval))) {

	display_interval = 200;
}
sscanf(argv[2],"%u",&display_interval);
printf("argc %d, arg3 %s, display interval %d \n",argc, argv[2], display_interval);

	/* Setup the socket */
	sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (sock_fd == -1)
		exit_program_err(-1, "socket");

	/* Initialize the address structs */
	memset(&proc_addr, 0, sizeof(struct sockaddr_nl));
	proc_addr.nl_family = AF_NETLINK;
	proc_addr.nl_pid = getpid();			// this process' PID
	proc_addr.nl_groups = CN_IDX_IWLAGN;
	memset(&kern_addr, 0, sizeof(struct sockaddr_nl));
	kern_addr.nl_family = AF_NETLINK;
	kern_addr.nl_pid = 0;					// kernel
	kern_addr.nl_groups = CN_IDX_IWLAGN;

	/* Now bind the socket */
	if (bind(sock_fd, (struct sockaddr *)&proc_addr, sizeof(struct sockaddr_nl)) == -1)
		exit_program_err(-1, "bind");

	/* And subscribe to netlink group */
	{
		int on = proc_addr.nl_groups;
		ret = setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &on, sizeof(on));
		if (ret)
			exit_program_err(-1, "setsockopt");
	}

	/* Set up the "caught_signal" function as this program's sig handler */
	signal(SIGINT, caught_signal);

	/* Poll socket forever waiting for a message */
	while (1)
	{
		/* Receive from socket with infinite timeout */
		ret = recv(sock_fd, buf, sizeof(buf), 0);
		if (ret == -1)
			exit_program_err(-1, "recvs");
		/* Pull out the message portion and print some stats */
		cmsg = NLMSG_DATA(buf);
		//if (count % SLOW_MSG_CNT == 0)
		//	printf("received %d bytes.\n", cmsg->len);
		/* Log the data to file */
		printf("received protocol 0x%X",cmsg->data[0]);
		l = (unsigned short) cmsg->len;
		l2 = htons(l);
		fwrite(&l2, 1, sizeof(unsigned short), out);
		ret = fwrite(cmsg->data, 1, l, out);
		if (count % display_interval == 0) {

                      printf("\b");
                      printf("\r");
                      printf("%db, cnt=%d", ret, count);
                      fflush(stdout);
                }
		++count;
		if (ret != l)
			exit_program_err(1, "fwrite");
	}

	exit_program(0);
	return 0;
}


FILE* open_file(char* filename, char* spec)
{
	FILE* fp = fopen(filename, spec);
	if (!fp)
	{
		perror("fopen");
		exit_program(1);
	}
	return fp;
}

void caught_signal(int sig)
{
	fprintf(stderr, "Caught signal %d\n", sig);
	exit_program(0);
}

void exit_program(int code)
{
	if (out)
	{
		fclose(out);
		out = NULL;
	}
	if (sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}
	exit(code);
}

void exit_program_err(int code, char* func)
{
	perror(func);
	exit_program(code);
}
