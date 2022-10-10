#include <sys/queue.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "nic_lib.h"

#define MAX_NET_SIZE 18

//set up datatables
uint8_t routeTable[3][MAX_NET_SIZE]; //id, next step towards, hops on this path
int rtHeight = 0;
int neighborTable[2][4]; //id, tick last heard. port 1 is idx 0, p2 if idx1
uint8_t myID;

int whatPort(uint8_t destID) {
	uint8_t nextStep; //find id of next step
	for (int i=0; i<rtHeight; i++) {
		if (routeTable[0][i] == destID) {
			nextStep = routeTable[1][i];
			break;
		}
	}
	int outPort; //find what port is that id
	for (int i=0; i<4; i++) {
		if (neighborTable[0][i] == nextStep) {
			outPort = i;
			break;
		}
	}
	return outPort;
}

void broadcastTable() {
	uint8_t output[rtHeight*3+4];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'u';
	int j=0; //indexing through datatable
	int i;
	for (i=4; i<rtHeight*3+4;i+=3) {
		output[i] = routeTable[0][j];
		output[i+1] = routeTable[1][j];
		output[i+2] = routeTable[2][j];
		j++;
	}
	broadcast(output,rtHeight*3+4);
}

void ping() {
	uint8_t output[5];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'p';
	output[4] = '\0';
	broadcast(output,5);
}

void newHere() {
	uint8_t output[5];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'n';
	output[4] = '\0';
	broadcast(output,5);
}

void deleteRoute(uint8_t deletedID) {
	uint8_t output[5];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'd';
	output[4] = deletedID;
	broadcast(output,5);
}
/*
 * Sends a packet with the 'a' type, to hold application layer data, as opposed to network layer
 * communications.
 * @param msg - pointer to an array of uint8_t bytes to be transmitted
 * @param length - how many bytes to read from msg
 * @param destID - Unique identifier to send message to
 */
void sendAppMsg(uint8_t* msg, int length, uint8_t destID) {
	uint8_t *output = malloc(sizeof(uint8_t)*(4+length));
	output[0] = myID;
	output[1] = 0;
	output[2] = destID;
	output[3] = 'a';
	for (int i=0; i<length; i++) {
		output[i+4] = msg[i];
	}
	//TODO mem leak
	sendMessage(whatPort(destID),output, length+4);
}

//TODO update accoridng to tony's revision
void recieveMessage(uint8_t* message, int port) {
	//decode header. may change/ be wrong
	uint8_t numBytes = message[0];
	uint8_t senderID = message[1];
	uint8_t hopCount = message[2];
	uint8_t destID = message[3];
	uint8_t msgType = message[4]; 
	uint8_t fromID = neighborTable[0][port];

	uint8_t tableChanged = 0;

	struct timespec time;
	switch (msgType){
		case 'p':
			printf("Recieved ping!\n");
			//ping
			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			neighborTable[1][port] = time.tv_sec;
			break;
	
		case 'n':
			printf("Recieved new neighbor!\n");
			//new pi started up. assuming they are our neighbor
			neighborTable[0][port] = senderID;

			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			neighborTable[1][port] = time.tv_sec;

			//set routing table
			routeTable[0][rtHeight] = senderID;
			routeTable[1][rtHeight] = senderID; //our neighbor
			routeTable[2][rtHeight] = 1;
			rtHeight++;
			tableChanged = 1;
			//send them our data table
			break;

		case 'u':
			//updated table
			printf("Recieved Updated Table\n");
			for (int i= 5;i<numBytes+1;i+=3) {
				uint8_t id = message[i];
				uint8_t next = message[i+1]; 
				uint8_t hops = message[i+2]; //hops to our neighbor, add 1 for us.
				uint8_t matchFound = 0;
				for (int j=0;j<rtHeight;j++) {
					if (id==routeTable[0][j]) {
						//we have this id in our datatable
						if(routeTable[2][j]>hops+1) {
							//our neighbor has a shorter route
							routeTable[1][j] = fromID;
							routeTable[2][j] = hops+1;
							tableChanged=1;
						}
						matchFound =1;
					}
				}
				if ((!matchFound) && (id!=myID) ) {
					routeTable[0][rtHeight] =id;
					routeTable[1][rtHeight] =next;
					routeTable[2][rtHeight] =hops+1;
					rtHeight++;
					tableChanged=1;
				}
			}

			break;

		case 'd': ;
			printf("recieved delete message\n");
			uint8_t deletedID = message[5];
			if (deletedID == myID) {
				//oh no they think i'm dead!
				newHere();
			}
			for (int i =0; i<rtHeight; i++) {
				if (routeTable[0][i]==deletedID) {
					if (routeTable[1][i]==senderID) {
						//delete entry and shift up
						rtHeight--;
						for (int j = i; j<rtHeight; j++) {
							routeTable[0][j] = routeTable[0][j+1];
							routeTable[1][j] = routeTable[1][j+1];
							routeTable[2][j] = routeTable[2][j+1];
						}
						deleteRoute(deletedID);
						//broadcasting tbl doesnt do anything on delete, so don't bother
					}
					else {
						tableChanged = 1;
					}
					break; //don't complete loop.
				}
			}
			break;

		case 'a':
			//application layer data. should forward here.
			if(destID == myID) {
				printf("Message for me:\n");
			}
			else {
				printf("Message for %d\n",destID);
				uint8_t output[numBytes];
				output[0] = senderID;
				output[1] = hopCount+1;
				output[2] = destID;
				output[3] = msgType;
				for (int i=5; i<numBytes+1; i++) {
					output[i-1] = message[i];
				}
				
				int outPort = whatPort(destID);
				sendMessage(outPort,output, numBytes);
			}
			for (int i= 5;i<numBytes+1;i++) {
				printf("%c",message[i]);
			}
			printf("\n");
			break;

		default:
			printf("Bad message type\n");
			break;
	}
	if (tableChanged) {
		broadcastTable();
	}
}



int main() {

	//0 out data tables
	for (int i =0;i<3;i++) {
		for (int j=0; j<MAX_NET_SIZE; j++) {
			routeTable[i][j] =0;
		}
	}
	for (int i =0;i<2;i++) {
		for (int j=0; j<4; j++) {
			neighborTable[i][j] =0;
		}
	}

	//get unique id
	printf("Enter unique identifier (0-255): ");
	char input[4];
	fgets(input,3,stdin);
	myID = (uint8_t) atoi(input);

	//register callback
	nic_lib_init(recieveMessage);

	newHere();

	int secSincePing;
	while (1) {
		sleep(1); //less than ideal, msgs can only go out every sec.
		secSincePing++;

		if (secSincePing>45) {
			ping();
			secSincePing =0;
		}
		if(secSincePing==15) {
			for (int i=0; i<4 ; i++) {
				uint8_t helloMsg[5];
				helloMsg[0]='h';
				helloMsg[1]='e';
				helloMsg[2]='l';
				helloMsg[3]='l';
				helloMsg[4]='o';
				if (neighborTable[1][i]!=0) {
					sendAppMsg(&helloMsg[0],5,neighborTable[0][i]);
				}
			}
			
			if (myID == 12) {
				uint8_t helloMsg[4];
				helloMsg[0]='h';
				helloMsg[1]='i';
				helloMsg[2]='1';
				helloMsg[3]='6';
				sendAppMsg(&helloMsg[0],4,16);
			}

			printf("Neighbors table:\nID\tLast Heard\n");
			for (int i=0;i<4;i++) {
				printf("%d\t%d\n",neighborTable[0][i],neighborTable[1][i]);
			}
			printf("\n");
			printf("Route table:\nID\tNext\tHops\n");
			for (int i=0;i<rtHeight;i++) {
				printf("%d\t%d\t%d\n",routeTable[0][i],routeTable[1][i],routeTable[2][i]);
			}
		}
		struct timespec time;
		clock_gettime(CLOCK_BOOTTIME, &time);
		for (int i=0; i<4; i++) {
			//if they every were alive, are they still?
			if ((time.tv_sec - neighborTable[1][i])>90 && neighborTable[1][i] > 0) { //they're dead!!
				uint8_t deletedID = neighborTable[0][i];
				deleteRoute(deletedID);
				//delete from neighbor table
				neighborTable[0][i] =0;
				neighborTable[1][i] =0;
				//delete from RT
				for (int i =0; i<rtHeight; i++) {
					if (routeTable[0][i]==deletedID) {
						if (routeTable[1][i]==deletedID) {
							//delete entry and shift up
							rtHeight--;
							for (int j = i; j<rtHeight; j++) {
								routeTable[0][j] = routeTable[0][j+1];
								routeTable[1][j] = routeTable[1][j+1];
								routeTable[2][j] = routeTable[2][j+1];
							}
							//broadcasting tbl doesnt do anything on delete, so don't bother
						}
					break; //don't complete loop.
					}
				}
			}
		}
	}
	return 0;
}
