# nic_net
The network layer for CP341: Computer Networking.

## Contents
This C project establishes the network layer for the custom CP341 network.
- d_net.c contains the network library.

## Building and Running
This code depends on our nic_link library, which is available [here](https://github.com/Jessicat-H/nic_link). To compile and run the code, run the following commands:
git clone https://github.com/Jessicat-H/nic_link.git
git clone https://github.com/Jessicat-H/nic_net
cd nic_net
bash library_script.sh
./src/sample

Then enter a unique number ID for every node in the network.

## Physical setup
The code is intended to work on any number of nodes up to 18. To connect a node to the network, connect the xmit and recv ports on one port to an open recv and xmit (respectively) on an open port on a Pi that is already in the network, then run the executable. There is currently no command-line interface, but the nodes will send each other pings and the code can be changed to transmit messages between nodes.

## Authors
The following code was written by [Jessica Hannebert](https://github.com/Jessicat-H), [Dylan Chapell](https://github.com/dylanchapell), and [Tony Mastromarino](https://github.com/tonydoesathing). 

