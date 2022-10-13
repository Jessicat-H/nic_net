#include <sys/queue.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nic_net.h"
#include <pthread.h>

#define MAX_NET_SIZE 18


//set up datatables
uint8_t routeTable[3][MAX_NET_SIZE]; //id, next step towards, hops on this path
int rtHeight = 0;
int neighborTable[2][4]; //id, tick last heard. port 1 is idx 0, p2 if idx1
uint8_t myID;
call_back messageReceived;
//lock for routing table so that it cannot be modified by two threads simultaneously
pthread_mutex_t lock;

/**
* Determine what port to send a message to if we want it to eventually get to the given destination
* @param destID the ID of the destination
*/
int whatPort(uint8_t destID) {
	printf("Route table:\nID\tNext\tHops\n");
		for (int i=0;i<rtHeight;i++) {
			printf("%d\t%d\t%d\n",routeTable[0][i],routeTable[1][i],routeTable[2][i]);
		}
	uint8_t nextStep; //find id of next step
	for (int i=0; i<rtHeight; i++) {
		if (routeTable[0][i] == destID) {
			nextStep = routeTable[1][i];
			break;
		}
	}
	int outPort = 5; //find what port is that id
	for (int i=0; i<4; i++) {
		if (neighborTable[0][i] == nextStep) {
			outPort = i;
			break;
		}
	}
	return outPort;
}

/**
* Broadcast our routing table to all our neighbors
*/
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

/**
* Send a ping message to let all our neighbors know we're alive
*/
void ping() {
	uint8_t output[5];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'p';
	output[4] = '\0';
	broadcast(output,5);
}

/**
* Send a message to let all our neighbors know we're new here
*/
void newHere() {
	uint8_t output[5];
	output[0] = myID;
	output[1] = 0;
	output[2] = UINT8_MAX;
	output[3] = 'n';
	output[4] = '\0';
	broadcast(output,5);
}

/**
* Send a message to let all our neighbors know we've lost access to a node
* @param deletedID the ID of the node we have lost our connection to
*/
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
	printf("sending message of size %d to %d\n", length,destID);
	output[0] = myID;
	output[1] = 0;
	output[2] = destID;
	output[3] = 'a';
	for (int i=0; i<length; i++) {
		output[i+4] = msg[i];
	}
	printf("the message: %s\n", &msg[1]);
	printf("Sending to port: %d\n", whatPort(destID));
	sendMessage(whatPort(destID),output, length+4);
	free(output);
}

/**
* Callback function to process any message received
* @param message the message received from the link layer, including all header data
* @param port the number of the port the message was received from
*/
void receiveMessage(uint8_t* message, int port) {
	//decode header. may change/ be wrong
	uint8_t numBytes = message[0];
	uint8_t senderID = message[1];
	uint8_t hopCount = message[2];
	uint8_t destID = message[3];
	uint8_t msgType = message[4]; 
	//uint8_t fromID = neighborTable[0][port];

	uint8_t tableChanged = 0;

	struct timespec time;
	switch (msgType){
		case 'p':
			printf("Received ping!\n");
			//ping
			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			pthread_mutex_lock(&lock);
			neighborTable[1][port] = time.tv_sec;
			neighborTable[0][port] = senderID;
			int matchFound = 0;
			for (int j=0;j<rtHeight;j++) {
				if (senderID==routeTable[0][j]) {
					//we have this id in our datatable
					if (routeTable[1][j] != senderID) {
						routeTable[1][j] = senderID;
						routeTable[2][j] = 1;
						tableChanged=1;
					}
					matchFound = 1;
				}
			}
			if ((!matchFound) && (senderID!=myID) ) {
				routeTable[0][rtHeight] =senderID;
				routeTable[1][rtHeight] =senderID;
				routeTable[2][rtHeight] =1;
				rtHeight++;
				tableChanged=1;
			}
			pthread_mutex_unlock(&lock);
			break;
	
		case 'n':
			printf("Received new neighbor!\n");
			//new pi started up. assuming they are our neighbor
			pthread_mutex_lock(&lock);
			neighborTable[0][port] = senderID;

			clock_gettime(CLOCK_BOOTTIME, &time);
			//set last heard to now
			neighborTable[1][port] = time.tv_sec;

			//set routing table
			routeTable[0][rtHeight] = senderID;
			routeTable[1][rtHeight] = senderID; //our neighbor
			routeTable[2][rtHeight] = 1;
			rtHeight++;
			pthread_mutex_unlock(&lock);
			tableChanged = 1;
			//send them our data table
			break;

		case 'u':
			//updated table
			printf("Received Updated Table\n");
			for (int i= 5;i<numBytes;i+=3) {
				uint8_t id = message[i];
				uint8_t next = message[i+1]; 
				uint8_t hops = message[i+2]; //hops to our neighbor, add 1 for us.
				uint8_t matchFound = 0;
				pthread_mutex_lock(&lock);
				for (int j=0;j<rtHeight;j++) {
					if (id==routeTable[0][j]) {
						//we have this id in our datatable
						if(routeTable[2][j]>hops+1) {
							//our neighbor has a shorter route
							routeTable[1][j] = senderID;
							routeTable[2][j] = hops+1;
							tableChanged=1;
						}
						matchFound =1;
					}
				}
				if ((!matchFound) && (id!=myID) ) {
					routeTable[0][rtHeight] =id;
					routeTable[1][rtHeight] =senderID;
					routeTable[2][rtHeight] =hops+1;
					rtHeight++;
					tableChanged=1;
				}
				pthread_mutex_unlock(&lock);
			}

			break;

		case 'd': ;
			printf("received delete message\n");
			uint8_t deletedID = message[5];
			if (deletedID == myID) {
				//oh no they think i'm dead!
				newHere();
			}
			pthread_mutex_lock(&lock);
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
			pthread_mutex_unlock(&lock);
			break;

		case 'a':
			//application layer data. should forward here.
			if(destID == myID) {
				printf("Message for me from %d:\n",senderID);
				messageReceived(message,message[5]);
			}
			else {
				printf("Message for %d from %d\n",destID, senderID);
				uint8_t output[numBytes];
				output[0] = senderID;
				output[1] = hopCount+1;
				output[2] = destID;
				output[3] = msgType;
				for (int i=5; i<numBytes+1; i++) {
					output[i-1] = message[i];
				}
				
				int outPort = whatPort(destID);
				if (outPort == 5) {
					printf("No port found, aborting\n");
				}
				else {
					sendMessage(outPort,output, numBytes);
				}
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
		printf("Route table:\nID\tNext\tHops\n");
		for (int i=0;i<rtHeight;i++) {
			printf("%d\t%d\t%d\n",routeTable[0][i],routeTable[1][i],routeTable[2][i]);
		}
		broadcastTable();
	}
}

/**
* Function to call to connect to the network
* @param id a unique identifier for this node
*/
void nic_net_init(int id) {
	
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

	myID = (uint8_t) id;
	//register callback
	nic_lib_init(receiveMessage);

	 if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return;
    }
	
	newHere();
	ping();

}

/**
* Iterate through list of neighbors to check whether they are still connected
* If not, remove them from all lists and notify the network
*/
void checkNeighbors() {
	struct timespec time;
	clock_gettime(CLOCK_BOOTTIME, &time);
	for (int i=0; i<4; i++) {
		//if they ever were alive, are they still?
		pthread_mutex_lock(&lock);
		if ((time.tv_sec - neighborTable[1][i])>90 && neighborTable[1][i] > 0) { //they're dead!!
			uint8_t deletedID = neighborTable[0][i];
			deleteRoute(deletedID);
			//delete from neighbor table
			neighborTable[0][i] =0;
			neighborTable[1][i] =0;
			//delete from RT
			for (int k =0; k<rtHeight; k++) {
				if (routeTable[0][k]==deletedID && routeTable[1][k]==deletedID) {
					printf("deleting neighbors\n");
					//delete entry and shift up
					rtHeight--;
					for (int j = k; j<rtHeight; j++) {
						routeTable[0][j] = routeTable[0][j+1];
						routeTable[1][j] = routeTable[1][j+1];
						routeTable[2][j] = routeTable[2][j+1];
					}
					break; //don't complete loop.
				}
			}
		}
		pthread_mutex_unlock(&lock);
	}

}

/**
 * Run pings
 * Called by runServer
*/
void* pingForever() {
	int numPings = 0;
	while (1) {
		sleep(10); //less than ideal, msgs can only go out every sec.
		numPings++;
		ping();
		if(numPings==3) {
			numPings=0;
			checkNeighbors();
		}
	}
}

/**
* Do server setup then infinitely loop, pinging and checking neighbors
* Will start up a seperate thread, which will never complete.
*/
int runServer(int id, call_back routerMessageReceived) {
	messageReceived = routerMessageReceived;
	nic_net_init(id);
	pthread_t thread_id;
    pthread_create(&thread_id, NULL, pingForever, NULL);
	return 0;
}
