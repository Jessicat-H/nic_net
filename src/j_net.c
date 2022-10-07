#include "nic_lib.h"
#include <sys/queue.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//attributes for routing
uint8_t myID;
uint8_t routingTable[54];
int routingTableLength;
int neighbors[4][2]; //one row per port; columns are id and time since last contacted

/**
 * I'm new here!
 * Sends a message to all neighbors to let them know that this node exists
 */
void newHere() {
	uint8_t numBytes = 5;
	uint8_t message[numBytes];
	//Message structure for this network: sender ID, hop count, destination ID, message type, [message]
	//sender ID
	message[0] = myID;
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
 * Update routing table based on getting a "I'm new here" message,
 * which we only receive from neighbors.
 * @param senderID the ID of the node sending the message
 */
void updateTableFromNew(uint8_t senderID) {
	for (int i = 0; i < routingTableLength; i+=3) { 
		if(routingTable[i] == senderID) { //if sender is already in the routing table
			routingTable[i+1] = senderID;
			routingTable[i+2] = 1;
			return;
		}
	}
	//destination, next, hop count
	routingTable[routingTableLength] = senderID;
	routingTable[routingTableLength+1] = senderID;
	routingTable[routingTableLength+2] = 1;
	routingTableLength += 3;
}

/**
 * Update table of neighbors based on getting a ping message,
 * which we only receive from neighbors.
 * @param port the port the ping came from
 * @param the ID of the node sending the ping
 */
void updateNeighborsFromPing(int port, uint8_t senderID) {
	struct timespec time;
	clock_gettime(CLOCK_BOOTTIME, &time);
	if(neighbors[port][0] != senderID) {
		printf("Updating sender ID for port %d\n",port);
		updateTableFromNew(senderID);
		neighbors[port][0] = senderID;
	}
	neighbors[port][1] = time.tv_sec;
}

/**
 * broadcast the routing table to all neighbors so they can update their own table
 */
void broadcastTable() {
	uint8_t message[4 + routingTableLength];
	//sender ID, hop count, destination ID, message type, [message]
	message[0] = myID;
	message[1] = 0;
	message[2] = UINT8_MAX;
	message[3] = 'u';
	for (int i=0; i<routingTableLength;i++) {
		message[i+4] = routingTable[i];
	}
	broadcast(message, 4 + routingTableLength);
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
 * Update our routing table from a routing table broadcast by another Pi
 * @param table the routing table sent by the other Pi
 * @param length the length of the table
 * @param senderID the ID of the node that sent the table
 */
int updateTableFromTable(uint8_t* table, uint8_t length, uint8_t senderID) {
	int tableChanged = 0;
	for (int i= 0;i<length;i+=3) {
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
		if (!matchFound && id != myID) { //we just discovered a new node in the network!
			routingTable[routingTableLength] = id;
			routingTable[routingTableLength + 1] = senderID;
			routingTable[routingTableLength + 2] = hops+1;
			routingTableLength += 3;
			tableChanged = 1;
		}
	}
	return tableChanged;
}

/**
 * Ping our neighbors to let them know we still exist
 */
void ping() {
	uint8_t message[5];
	//sender ID, hop count, destination ID, message type, [message]
	message[0] = myID;
	message[1] = 0;
	message[2] = UINT8_MAX;
	message[3] = 'p';
	message[4] = '\0';
	broadcast(message, 5);
}

/**
 * Update the routing table if we receive a delete signal
 * @param nodeID: the node that was deleted
 * @param senderID: the node signaling the deletion
 * @return whether the table was updated due to the delete operation
 */
int updateTableFromDelete(uint8_t nodeID, uint8_t senderID) {
	for (int i = 0; i < routingTableLength; i+=3) {
		if(routingTable[i]==nodeID) { //identify the node that was deleted
			if(routingTable[i+1] == senderID) { //if we have a route through the sender to the dead node
				for(int j = i; j<routingTableLength; j+=3) { //delete the entry by shifting everything over
					routingTable[j] = routingTable[j+3];
					routingTable[j+1] = routingTable[j+4];
					routingTable[j+2] = routingTable[j+5];
				}
				routingTableLength -= 3;
				return(1);
			}
		}
	}
	return(0);
}


/**
 * Send a message to tell the network we've lost our connection to a node
 * @param the node whose connection has been lost
 */
void sendDelete(uint8_t nodeID) {
	uint8_t message[6];
	//sender ID, hop count, destination ID, message type, [message]
	message[0] = myID;
	message[1] = 0;
	message[2] = UINT8_MAX;
	message[3] = 'd';
	message[4] = nodeID;
	message[5] = '\0';
	broadcast(message, 6);
}

/*
	call back for receiving a message
	@param message - the message received
	@param port - the port the message was sent to
*/
void messageReceived(uint8_t* message, int port) {
	//# of bytes, sender ID, hop count, destination ID, message type, [message]
	uint8_t byteCount = message[0];
	uint8_t senderID = message[1];
	uint8_t hopCount = message[2];
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
			if(!updateTableFromTable(&message[5],byteCount-5,senderID)) {
				broadcastTable();
			}
			break;
		case('a'): //application message
			printf("Received app message\n");
			//TODO need code here to relay message if it's not meant for us
			printf("%s\n",&message[5]);
			break;
		case('d'):
			printf("Received disconnect signal\n");
			if(message[5] == myID) { //if we receive news of our own deletion
				printf("Received self-disconnect signal. Reconnecting node...\n");
				newHere(); //correct the mistake
			}
			else if(updateTableFromDelete(message[5], senderID)) { //if delete results in an updated table
				sendDelete(message[5]);
			}
			else { //if delete does not result in an updated table
				broadcastTable();
			}
			break;
	}
}

/**
 * Check our neighbor table to make sure we haven't lost a connection with any of them
 */
void refresh_neighbors() {
	struct timespec time;
	clock_gettime(CLOCK_BOOTTIME, &time);
	for (int i = 0; i < 4; i++) {
		if(neighbors[i][0] != 0) {
			if (time.tv_sec - neighbors[i][1] > 10) { //if more than two ping intervals have passed since last ping
				//assume node has been disconnected/deleted
				updateTableFromDelete(neighbors[i][0],neighbors[i][0]);
				sendDelete(neighbors[i][0]);
				neighbors[i][0] = 0;
				neighbors[i][1] = 0;
			}
		}
	}
}

/**
 * Initialize the network connection
 */
void init() {
	//initialize neighbors table
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 2; j++) {
			neighbors[i][j] = 0;
		}
	}
	printf("Enter unique identifier (1-255): ");
	char IDinput[4];
	fgets(IDinput, 3, stdin);
	myID = (uint8_t) atoi(IDinput);
	//initialize message queue
	routingTableLength = 0;
	nic_lib_init(messageReceived);
	newHere();
}

/**
 * Broadcast a message to chat (to all known nodes in the network)
 * @param message: The message to send
 */
void broadcastChat(char* message) {
	for(int i = 0; i < routingTableLength; i+=3) {
		uint8_t msg[6+strlen(message)];
		msg[0] = myID;
		msg[1] = 0;
		msg[2] = UINT8_MAX;
		msg[3] = 'a';
		msg[4] = routingTable[i];
		for(int j=0; j<=strlen(message);j++) {
			msg[j+5] = (uint8_t) message[j];
		}
		uint8_t len = 6+strlen(message);
		uint8_t port = portFromID(routingTable[i+1]);
		//add to queue
		sendMessage(port, msg, len);

	}
}

/**
 * Start the network and send messages
 */
int main() {
	struct timespec time;
	int last_ping;
	init();
	last_ping = 0;
	while(1) {
		//send all pending messages
		//sendFromQueue();
		clock_gettime(CLOCK_BOOTTIME, &time);
		//every 5 seconds, let everyone know we're still here
		if (time.tv_sec - last_ping >= 5) {
			printf("Routing table length: %d\n",routingTableLength);
			ping();
			last_ping = time.tv_sec;
			//update table depending on lack of pings
			refresh_neighbors();
			broadcastChat("yay");
		}
	}
}
