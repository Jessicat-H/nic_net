#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//struct to put messages in queue
struct msgEntry {
	uint8_t* msg;
	TAILQ_ENTRY(msgEntry) msgEntries;
};

//initialize the message queue
TAILQ_HEAD(tailhead, msgEntry);
struct tailhead head;

//pointer to message queue
struct msgEntry *mp;

//attributes for routing
uint8_t id;
uint8_t* routingTable;
int routingTableLength;
int neighbors[4][2]; //one row per port; columns are id and time since last contacted

void updateTableFromNew(uint8_t senderID) {
	routingTable = realloc(routingTable, routingTableLength+3);
	//destination, next, hop count
	routingTable[routingTableLength] = senderID;
	//strcat(routingTable,senderID);
	//if the node is a neighbor, the easiest way to send a message is just to send it straight there
	routingTable[routingTableLength+1] = senderID;
	//strcat(routingTable,senderID);
	//and it only takes one hop!
	routingTable[routingTableLength+2] = 1;
	//strcat(routingTable,1);
	routingTableLength += 3;
}

void updateNeighborsFromPing(int port, uint8_t senderID) {
	struct timespec time;
	clock_gettime(CLOCK_BOOTTIME, &time);
	if(neighbors[port][0] != senderID) {
		printf("Updating sender ID for port %d\n",port);
		neighbors[port][0] = senderID;
	}
	neighbors[port][1] = time.tv_sec;
}

void broadcastTable() {
	uint8_t numBytes = 4 + routingTableLength;
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	message[0] = id;
	//hop count
	message[1] = 0;
	//destination: special character for all neighbors
	message[2] = UINT8_MAX;
	//message type: table
	message[3] = 'u';
	//the table itself
	for (int i=0; i<routingTableLength;i++) {
		message[i+4] = routingTable[i];
	}
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
	if (!TAILQ_EMPTY(&head)) {
		TAILQ_FOREACH(mp, &head, msgEntries) {
			//get the destination
			uint8_t destID = mp->msg[3];
			for (int i=0; i<routingTableLength;i+=3) {
				if(routingTable[i]==destID) {
					//second char is the next node to pass the message on to
					int nextNode = (int) routingTable[i+1];
					int port = portFromID(nextNode);
					if (port==5) {
						printf("Neighbor %d not found, cannot send message\n",nextNode);
					}
					else{
						//sendMessage(portFromID(nextNode),&(mp->msg[5]),strlen(&(mp->msg[5])));
					}
				}
			}
		;}
	}
}

int updateTableFromTable(uint8_t* table, uint8_t length, uint8_t senderID) {
	int tableChanged = 0;
	for (int i= 5;i<length;i+=3) {
		uint8_t id = table[i];
		uint8_t hops = table[i+2]; //hops to our neighbor, add 1 for us.
		uint8_t matchFound = 0;
		for (int j=0;j<routingTableLength;j+=3) {
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
	message[0] = id;
	//hop count
	message[1] = 0;
	//destination: special character for all neighbors
	message[2] = UINT8_MAX;
	//message type: table
	message[3] = 'p';
	//send it along!
	message[4] = '\0';
	broadcast(message, numBytes);
}

/*
	call back for receiving a message
	@param message - the message received
*/
void messageReceived(uint8_t* message, int port) {
	//Message structure for this network: # of bytes, sender ID, hop count, destination ID, message type, [message]
	uint8_t byteCount = message[0];
	uint8_t senderID = message[1];
	//uint8_t hopCount = message[2];
	//uint8_t destID = message[3];
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
			if(updateTableFromTable(&message[5],byteCount-5,senderID)) {
				broadcastTable();
			}
			break;
		case('a'): //application message
			printf("Received app message\n");
			break;
		case('d'):
			printf("Received delete signal\n");
			if(updateTableFromDelete(message[5])) { //if delete results in an updated table
				sendDelete()
			}
			break;
	}
}

/**
 * I'm new here!
 * Sends a message to all neighbors to let them know that this node exists
 */
void new_here() {
	uint8_t numBytes = 5;
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	message[0] = id;
	//hop count
	message[1] = 0;
	//destination: special character for all neighbors
	message[2] = UINT8_MAX;
	//message type: table
	message[3] = 'n';
	//send it along!
	message[4] = '\0';
	broadcast(message, numBytes);
}

/**
 * Send a message to tell the network we've lost our connection to a node
 * @param the node whose connection has been lost
 */
void sendDelete(uint8_t nodeID) {
	uint8_t numBytes = 6;
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	message[0] = id;
	//hop count
	message[1] = 0;
	//destination: special character for all neighbors
	message[2] = UINT8_MAX;
	//message type: table
	message[3] = 'd';
	//send it along!
	message[4] = 'nodeID';
	message[5] = '\0';
	broadcast(message, numBytes);
}

void refresh_neighbors() {
	for (int i = 0; i < 4; i++) {
		if (neighbors[i][])
	}
}

int main() {
	struct timespec time;
	int last_ping;
	//initialize neighbors table
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			neighbors[i][j] = 0;
		}
	}
	printf("Enter unique identifier (0-255): ");
	char IDinput[4];
	fgets(IDinput, 3, stdin);
	id = (uint8_t) atoi(IDinput);
	//initialize message queue
	TAILQ_INIT(&head);
	routingTableLength = 0;
	nic_lib_init(messageReceived);
	new_here();
	last_ping = 0;
	while(1) {
		//send all pending messages
		sendFromQueue();
		clock_gettime(CLOCK_BOOTTIME, &time);
		//every 5 seconds, let everyone know we're still here
		if (time.tv_sec - last_ping >= 5) {
			ping();
			last_ping = time.tv_sec;
			//update table depending on lack of pings
			refresh_neighbors();
		}
	}
	free(routingTable);
}
