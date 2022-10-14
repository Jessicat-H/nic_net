#include "nic_net.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <poll.h>

#define PORT_PATH "/tmp/nic_net"
#define SIZE 123
#define MAX_CLIENTS 10

int appIDTable[MAX_CLIENTS][2] = {0}; //[row][col] AppId, File Descriptor
struct pollfd pollfds[MAX_CLIENTS + 1];
int numClients = 0;

void routerMessageReceived(uint8_t* message, int appID) {
    // get entire header
    for (int i=0; i<numClients; i++) {
        if (appIDTable[i][0]==appID) {
            for(int i=0;i<message[0]+1;i++){
                printf("%d ",message[i]);
            }
            printf("\n");
            write(appIDTable[i][1], message, message[0]+1);
            break;
        }
    }
}

/*
* Code adapted from 
* https://github.com/weboutin/simple-socket-server/blob/master/poll-server.c
*/
int main(int argc, char** argv) {
    //initialize app id table
    for (int i = 0; i<10; i++) {
        appIDTable[i][0] = 0;
        appIDTable[i][1] = 0;
    }

    if (argc < 2) {
        printf("Usage: ./router <node IP>");
        return(1);
    }
    int myID = atoi(argv[1]);

        runServer(myID, routerMessageReceived);
        /* Startup server */
        remove(PORT_PATH);
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
        //listen for regular and high priority data
        pollfds[0].fd = serverSock;
        pollfds[0].events = POLLIN | POLLPRI;

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
                    socklen_t addrLen = sizeof(cliAddr);
                    int clientSock = accept(serverSock, (struct sockaddr *)&cliAddr, &addrLen);

                    printf("accept success\n");
                    //put the client in the list of sockets we r watching
                    numClients++;
                    pollfds[numClients].fd = clientSock;
                    pollfds[numClients].events = POLLIN | POLLPRI;
                    appIDTable[numClients][1] = clientSock;

                }
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
                                appIDTable[j-1][0] = appIDTable[j][0];
                                appIDTable[j-1][1] = appIDTable[j][1];
                            }
                            pollfds[numClients+1].fd =0;
                            pollfds[numClients+1].events =0;
                            pollfds[numClients+1].revents =0;
                            appIDTable[numClients][0] = 0;
                            appIDTable[numClients][1] = 0;
                        }
                        else {
                            if (appIDTable[i-1][1]==0) { //first message from app
                                appIDTable[i-1][0] = buf[0];
                                appIDTable[i-1][1] = pollfds[i].fd;
				printf("router; new message received\n");
                            }
                            else { //not first msg
                                uint8_t dest = buf[0];
                                uint8_t appID = buf[1];
                                uint8_t msgLength = bufSize - 1;

				    printf("Dest: %d;  AppID: %d;  msg:%s\n", dest, appID, &buf[2]);
                                uint8_t output[SIZE];
                                for (int g=1; g<bufSize; g++) {
                                    output[g-1] = buf[g];
                                }

                                for(int i=0;i<msgLength;i++){
                                    printf("%d ",output[i]);
                                }
                                printf("\n");

                                sendAppMsg(output, msgLength, dest);
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
