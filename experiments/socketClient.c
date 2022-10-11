#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

//#define PORT 9990
#define PORT_PATH "/tmp/nic_net"
#define SIZE 123


/*
* Code adapted from https://github.com/weboutin/simple-socket-server/blob/master/client.c
*
*/
int main()
{
    int client_socket = socket(AF_LOCAL, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, PORT_PATH);

    int server_socket = connect(client_socket, (struct sockaddr *)&addr, sizeof(addr));

    printf("Connect result ==> %d\n Client Sock ==> %d\n", server_socket, client_socket);
    if (server_socket < 0) {
        perror("Connect result");
        exit(1);
    }

    char buf[SIZE] = {0};

    while (1)
    {
        printf("Send to server: ");
        scanf("%s", buf);
        write(client_socket, buf, strlen(buf));
        printf("\n");
        if (strncmp(buf, "end", 3) == 0)
        {
            break;
        }
    }
    close(server_socket);

    return 0;
}