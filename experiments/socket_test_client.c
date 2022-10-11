#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * Inter Process Communication client
 */

typedef enum {
	IPC_OK = 0,
	IPC_UNKNOWN,
	IPC_SOCKET_ERROR,
	IPC_CONNECT_FAILED
} ipc_status;

// s is socket
// t is received number of bytes
// len is address length
int s, t, len;
// server address
struct sockaddr_un remote;

/* status */
ipc_status error;

void ipc_connect(const char *path) {
	error = IPC_UNKNOWN;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s == -1) {
		error = IPC_SOCKET_ERROR;
		return;
	}
	
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, path);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		error = IPC_CONNECT_FAILED;
		return;
	}
	
	error = IPC_OK;
	return;
}

/* sends ipc data */
int ipc_client_send(char *buf, int buflen) {
	return send(s, buf, buflen, 0);
}

/* read ipc data */
int ipc_client_read(char *buf, int buflen) {
	t = recv(s, buf, buflen, 0);
	return t > 0;
}

/* close ipc socket */
void ipc_client_close() {
	close(s);
}

void handle_error(ipc_status err) {
	switch (err) {
	case IPC_SOCKET_ERROR:
		puts("socket call failed");
		break;
	case IPC_CONNECT_FAILED:
		puts("socket connection failed. is the daemon running?");
		break;
	};
}

int main(int argc, char *argv[]) {
	
	/* connect to socket*/
	ipc_connect("echo_socket");
	if (error) {
		handle_error(error);
		return 1;
	}

	char str[128];
	while (printf(">"), fgets(str, 128, stdin), !feof(stdin)) {
		ipc_client_send(str, strlen(str));

		if (ipc_client_read(str, 128)) {
			printf("server> %s", str);
		} else {
			if (t < 0) puts("error on recv");
			else
				puts("Server closed connection");
			return 1;
		}
	}
	ipc_client_close();

	return 0;
}