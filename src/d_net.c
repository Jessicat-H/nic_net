//#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

struct msgEntry {
	char* msg;
	TAILQ_ENTRY(msgEntry) msgEntries;
};

//TODO update accoridng to tony's revision
void recieveMessage(char* message, int port) {
	return;
}

TAILQ_HEAD(tailhead, msgEntry);

int main() {
	//build FIFO Queue to hold messages before they are sent
	struct msgEntry m1,m2,m3;
	
	struct tailhead head;

	TAILQ_INIT(&head);

	m1.msg = "Hello world";
	m2.msg = "Beaotiful Day Innit!";
	m3.msg = "Ello Wankas!";

	//insert at tail of queue
	TAILQ_INSERT_TAIL(&head, &m1, msgEntries);
	TAILQ_INSERT_TAIL(&head, &m2, msgEntries);
	TAILQ_INSERT_TAIL(&head, &m3, msgEntries);
	
	struct msgEntry *mp;
	TAILQ_FOREACH(mp, &head, msgEntries)
		printf("%s\n",mp->msg);
	return 0;
}