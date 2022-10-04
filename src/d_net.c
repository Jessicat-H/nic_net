#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

struct msgEntry {
	int port;
	char* msg;
	TAILQ_ENTRY(msgEntry) msgEntries;
};

//set up datatables
char routeTable[3][18]; //id, next step towards, hops on this path
int neighborTable[3][4]; //port, id, tick last heard

int getNeighborID(int port) {
	//TODO
	return -1;
}

//TODO update accoridng to tony's revision
void recieveMessage(char* message, int port) {
	//decode header. may change/ be wrong
	char numBytes = message[0];
	char senderID = message[1];
	char hopCount = message[2];
	char destID = message[3];
	char msgType = message[4]; 
	int fromID = getNeighborID(port);

	switch (msgType){
		case 'p':
			//ping
			
			break;
	
		case 'n':
			//new pi started up
			break;

		case 'u':
			//updated table
			break;

		case 'a':
			//application layer data
			break;

		default:
			break;
	}
}

TAILQ_HEAD(tailhead, msgEntry);

int main() {
	//build FIFO Queue to hold messages before they are sent
	struct tailhead head;
	TAILQ_INIT(&head);

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