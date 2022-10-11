/**
* Determine what port to send a message to if we want it to eventually get to the given destination
* @param destID the ID of the destination
*/
int whatPort(uint8_t destID);

/**
* Broadcast our routing table to all our neighbors
*/
void broadcastTable();


/**
* Send a ping message to let all our neighbors know we're alive
*/
void ping();

/**
* Send a message to let all our neighbors know we're new here
*/
void newHere();

/**
* Send a message to let all our neighbors know we've lost access to a node
* @param deletedID the ID of the node we have lost our connection to
*/
void deleteRoute(uint8_t deletedID);

/*
 * Sends a packet with the 'a' type, to hold application layer data, as opposed to network layer
 * communications.
 * @param msg - pointer to an array of uint8_t bytes to be transmitted
 * @param length - how many bytes to read from msg
 * @param destID - Unique identifier to send message to
 */
void sendAppMsg(uint8_t* msg, int length, uint8_t destID);

/**
* Callback function to process any message received
* @param message the message received from the link layer, including all header data
* @param port the number of the port the message was received from
*/
void recieveMessage(uint8_t* message, int port);

/**
* Function to call to connect to the network
* @param id a unique identifier for this node
*/
void nic_net_init(int id);

/**
* Iterate through list of neighbors to check whether they are still connected
* If not, remove them from all lists and notify the network
*/
void checkNeighbors();

