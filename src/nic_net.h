#ifndef NIC_NET
#define NIC_NET

#include <unistd.h>
#include "nic_lib.h"

/*
 * Sends a packet with the 'a' type, to hold application layer data, as opposed to network layer
 * communications.
 * @param msg - pointer to an array of uint8_t bytes to be transmitted
 * @param length - how many bytes to read from msg
 * @param destID - Unique identifier to send message to
 */
void sendAppMsg(uint8_t* msg, int length, uint8_t destID);

/**
* Do server setup then infinitely loop, pinging and checking neighbors
* Should run in a seperate thread, will never complete.
*/
int runServer(int id, call_back routerMessageRecieved);

#endif