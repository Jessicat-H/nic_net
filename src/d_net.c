#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

struct msgEntry {
	int port;
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
	struct tailhead head;
	TAILQ_INIT(&head);

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