#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

struct pollfd fds[200];
int    nfds = 1, current_size = 0, i, j, rc;

// socket id
unsigned int s;
// client socket id
unsigned int s2;
// local address
struct sockaddr_un local;
// client address
struct sockaddr_un remote;
// length of the message
unsigned int msglen;




/* create unix domain socket inter process communication server */
void ipc_create(const char *path) {
	s = socket(AF_UNIX, SOCK_STREAM, 0);

    // rc = ioctl(s, FIONBIO, (char *)&on);
    // if (rc < 0)
    // {
    //     perror("ioctl() failed");
    //     close(listen_sd);
    //     exit(-1);
    // }
	
	/* bind */
	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, path);
	unlink(local.sun_path);
	msglen = strlen(local.sun_path) + sizeof(local.sun_family);
	
	bind(s, (struct sockaddr *)&local, msglen);

	/* listen */
	listen(s, 5);
}

/* accept ipc client */
void ipc_server_accept() {
	msglen = sizeof(remote);
	s2 = accept(s, (struct sockaddr *)&(remote), &msglen);
}

/* read from ipc server */
int ipc_server_read(char *buf, int buflen) {
	msglen = recv(s2, buf, buflen, 0);
	return msglen > 0;
}

/* write to ipc client */
void ipc_server_send(char *buf, int buflen) {
	send(s2, buf, buflen, 0);
}

/* close srv */
void ipc_server_close() {
	close(s2);
}

int main(int argc, char *argv[]) {
	
	ipc_create("echo_socket");
	
	char buf[128];
	while (1) {
		/* call accept */
		puts("waiting for connection");
		ipc_server_accept();
		puts("connected!");

		/* handle connection and loop back to accept*/
		while (ipc_server_read(buf, 128)) {
			printf("-> '%.*s'\n", msglen - 1, buf);
			if (!strncmp(buf, "end", 3)) {
				puts("closing");
				ipc_server_close();
				exit(0);
			}

			ipc_server_send(buf, msglen);
		}
		puts("client closed connection");
	}

	return 0;
}