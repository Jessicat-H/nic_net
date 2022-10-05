#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

//struct to put messages in queue
struct msgEntry {
	uint8_t* msg;
	TAILQ_ENTRY(msgEntry) msgEntries;
};

//initialize the message queue
TAILQ_HEAD(tailhead, msgEntry);
struct tailhead head;
TAILQ_INIT(&head);

//pointer to message queue
struct msgEntry *mp;

//attributes for routing
uint8_t id;
uint8_t* routingTable;
int neighbors[4][2]; //one row per port; columns are id and time since last contacted

/*
	call back for receiving a message
	@param message - the message received
*/
void messageReceived(uint8_t* message, int port) {
	//Message structure for this network: # of bytes, sender ID, hop count, destination ID, message type, [message]
	uint8_t byteCount = message[0];
	uint8_t senderID = message[1];
	uint8_t hopCount = message[2];
	uint8_t destID = message[3];
	uint8_t msgType = message[4];
	switch(msgType) {
		case('p'): //regular ping
			printf("Received ping\n");
			//update our list of neighbors so we know we've heard from this neighbor recently
			updateNeighborsFromPing(port, senderID);
			break;
		case('n'): //"I'm new here" signal
			printf("Received new here\n");
			//add the new neighbor to our routing table
			updateTableFromNew(senderID);
			//add to our list of neighbors
			updateNeighborsFromPing(port, senderID);
			//broadcast the updated table to all neighbors
			broadcastTable();
			break;
		case('u'): //updated routing table from a neighbor
			printf("Received updated table\n");
			if(updateTableFromTable(&message[5],byteCount-5, senderID)) {
				broadcastTable();
			}
			break;
		case('a'): //application message
			printf("Received app message\n");
			break;
	}
}

void updateTableFromNew(uint8_t senderID) {
	free(routingTable);
	routingTable = realloc((strlen(routingTable))+4);
	//destination, next, hop count
	strcat(routingTable,senderID);
	//if the node is a neighbor, the easiest way to send a message is just to send it straight there
	strcat(routingTable,senderID);
	//and it only takes one hop!
	strcat(routingTable,1);
}

void updateNeighborsFromPing(int port, uint8_t senderID) {
	struct timespec time;
	clock_gettime(CLOCK_BOOTTIME, &time);
	if(neighbors[port][0] != senderID) {
		printf("Updating sender ID for port %d\n",port);
	}
	neighbors[port][1] = time.tv_sec;
}

void broadcastTable() {
	uint8_t numBytes = 5 + strlen(routingTable);
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	strcpy(message, id);
	//hop count
	strcpy(message, 0);
	//destination: special character for all neighbors
	strcpy(message, UINT8_MAX);
	//message type: table
	strcpy(message, 'u');
	//the table itself
	strcpy(message, routingTable);
	broadcast(message, numBytes);
}

/**
 * Determine from a given node identifier which port it corresponds to
 * @param id: the node's identifier
 */
int portFromID(int id) {
	for (int i=0; i<4; i++) {
		if(neighbors[i][0] == id) {
			return(neighbors[i][1]);
		}
	}
	return(5); //error code, neighbor not found
}

/**
 * Send all messages from the message queue
 */
void sendFromQueue() {
	TAILQ_FOREACH(mp, &head, msgEntries) {
		//get the destination
		uint8_t destID = mp->msg[3];
		for (int i=0; i<strlen(routingTable);i+=3) {
			if(routingTable[i]==destID) {
				//second char is the next node to pass the message on to
				int nextNode = (int) routingTable[i+1];
				int port = portFromID(nextNode);
				if (port==5) {
					printf("Neighbor %d not found, cannot send message\n",nextNode);
				}
				else{
					sendMessage(portFromID(nextNode),&(mp->msg[5]),strlen(&(mp->msg[5])));
				}
			}
		}
		;}
}

int updateTableFromTable(char* table, uint8_t length, uint8_t senderID) {
	int tableChanged = 0;
	for (int i= 5;i<length;i+=3) {
		uint8_t id = table[i];
		uint8_t next = table[i+1]; 
		uint8_t hops = table[i+2]; //hops to our neighbor, add 1 for us.
		uint8_t matchFound = 0;
		for (int j=0;j<(strlen(routingTable)-1);j+=3) {
			if (id==routingTable[j]) {
				//we have this id in our datatable
				if(routingTable[j+2]>hops+1) {
					//our neighbor has a shorter route
					routingTable[j+1] = senderID;
					routingTable[j+2] = hops+1;
					tableChanged = 1;
				}
				matchFound = 1;
			}
		}
		if (!matchFound) {
			updateTableFromNew(senderID);
			tableChanged = 1;
		}
	}
	return tableChanged;
}

void ping() {
	uint8_t numBytes = 5;
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	strcpy(message, id);
	//hop count
	strcpy(message, 0);
	//destination: special character for all neighbors
	strcpy(message, UINT8_MAX);
	//message type: table
	strcpy(message, 'p');
	//send it along!
	broadcast(message, numBytes);
}

int main() {
	//initialize neighbors table
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			neighbors[i][j] = 0;
		}
	}
	printf("Enter unique identifier: ");
	uint8_t IDinput[3];
	fgets(IDinput, 3, stdin);
	id = (uint8_t) atoi(IDinput);
	routingTable = malloc(sizeof(uint8_t)*3);
	nic_lib_init(messageReceived);
	while(1) {
		sendFromQueue();
		//update table depending on lack of pings
		//ping
		sleep(1);
		ping();
	}
	free(routingTable);
}
