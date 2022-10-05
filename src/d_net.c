#include "nic_lib.h"
#include <sys/queue.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

struct msgEntry {
	int port;
	char* msg;
	TAILQ_ENTRY(msgEntry) msgEntries;
};

//set up datatables
char routeTable[3][18]; //id, next step towards, hops on this path
int rtHeight = 0;
int neighborTable[2][4]; //id, tick last heard. port 1 is idx 0, p2 if idx1

//TODO update accoridng to tony's revision
void recieveMessage(char* message, int port) {
	//decode header. may change/ be wrong
	char numBytes = message[0];
	char senderID = message[1];
	char hopCount = message[2];
	char destID = message[3];
	char msgType = message[4]; 
	int fromID = neighborTable[0][port-1];

	uint8_t tableChanged =0;
	switch (msgType){
		case 'p':
			//ping
			struct timespec time;
			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			neighborTable[1][port-1] = time.tv_sec;
			break;
	
		case 'n':
			//new pi started up. assuming they are our neighbor
			neighborTable[0][port-1] = senderID;

			struct timespec time;
			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			neighborTable[1][port-1] = time.tv_sec;

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
				if (!matchFound) {
					routeTable[0][rtHeight] =id;
					routeTable[1][rtHeight] =next;
					routeTable[2][rtHeight] =hops+1;
					rtHeight++;
					tableChanged=1;
				}
			}

			break;

		case 'a':
			//application layer data. should forward here.
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

TAILQ_HEAD(tailhead, msgEntry);

int main() {
	//build FIFO Queue to hold messages before they are sent
	struct tailhead head;
	TAILQ_INIT(&head);

	//0 out data tables
	for (int i =0;i<3;i++) {
		for (int j=0; j<18; j++) {
			routeTable[i][j] =0;
		}
	}
	for (int i =0;i<2;i++) {
		for (int j=0; j<4; j++) {
			neighborTable[i][j] =0;
		}
	}
	//register callback
	nic_lib_init(recieveMessage);

	while (1) {
		//if there are messages to be sent, send them (FIFO)
		if (!TAILQ_EMPTY(&head)) {
			struct msgEntry *mp;
			TAILQ_FOREACH(mp, &head, msgEntries)
				sendMessage(mp->port,mp->msg);
		}
	}
	return 0;
}