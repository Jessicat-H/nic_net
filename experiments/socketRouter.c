#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <poll.h>

#define PORT_PATH "/tmp/nic_net"
#define SIZE 123
#define MAX_CLIENTS 10

/*
* Code adapted from 
* https://github.com/weboutin/simple-socket-server/blob/master/poll-server.c
* 
*/
int main(int argc, char** argv) {
    /* Startup server */
    int serverSock = socket(AF_LOCAL, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, PORT_PATH);

    if (bind(serverSock, (struct sockaddr *)&addr, sizeof(addr)) <0) {
        printf("Error on bind\n");
        return -1;
    }

    if (listen(serverSock, 5) < 0) {
        printf("Error on listen\n");
        return -1;
    }

    printf("server started\n");

    /* Server loop */
    struct pollfd pollfds[MAX_CLIENTS + 1];
    //listen for regular and high priority data
    pollfds[0].fd = serverSock;
    pollfds[0].events = POLLIN | POLLPRI;
    int numClients = 0; //what is this? num clients?

    while (1) {
        //hang until there is data to read
        if (poll(pollfds, numClients + 1, -1)<0) {
            printf("Error on poll\n");
            return -1;
        }   
        else {
            //if there is info on the server's port
            if (pollfds[0].revents & POLLIN) {
                struct sockaddr_un cliAddr;
                int addrLen = sizeof(cliAddr);
                int clientSock = accept(serverSock, (struct sockaddr *)&cliAddr, &addrLen);

                printf("accept success\n");
                //put the client in the list of sockets we r watching
                numClients++;
                pollfds[numClients].fd = clientSock;
                pollfds[numClients].events = POLLIN | POLLPRI;

            }
            //could MAX_CLIENTS instead be useClient?
            for (int i =1; i < numClients + 1; i++) {
                if (pollfds[i].fd > 0 && pollfds[i].revents & POLLIN) {
                    uint8_t buf[SIZE]; //enough for app id + max msg
                    int bufSize = read(pollfds[i].fd, buf, SIZE - 1);
                    if (bufSize == -1 || bufSize == 0) {
                        //read from client failed, remove it by shifting up
                        numClients--;
                        for (int j = i; j<numClients+1; j++) {
							pollfds[j].fd = pollfds[j+1].fd;
                            pollfds[j].events = pollfds[j+1].events;
                            pollfds[j].revents = pollfds[j+1].revents;
						}
                        pollfds[numClients+1].fd =0;
                        pollfds[numClients+1].events =0;
                        pollfds[numClients+1].revents =0;
                    }
                    else {
                        buf[bufSize] = '\0';
                        printf("From client: %s\n", buf);
                        for(int j=1;j<numClients+1;j++){
                            write(pollfds[j].fd, buf, bufSize+1);
                        }
                    }
                }
            }

        }
    }
    printf("server end\n");
    close(serverSock);

    return 0;
}
