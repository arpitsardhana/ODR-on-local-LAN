Created By		:		Arpit Singh,Ipshita Singha Roy
Student Id		:		Arpit Singh (110162005),Ipshita Singha Roy (110284325)
Date			:		Nov 22 2015
File Type		:		README
Summary			:		The aim of this program is to implement an On-Demand shortest-hop Routing (ODR) protocol for routing messages in networks of fixed but arbitrary and unknown connectivity through a client/server time application in which clients and servers communicate with each other across a network using PF_PACKET sockets and Unix domain datagram sockets. The implementation is based on (a simplified version of) the AODV algorithm.


Note - Program works correctly if tested sequentially excluding node no 5.On full scale it often breaks. It works for full topology if nde 5 is not included. If it hanges, please restart odr and server nodes.
I have tested it first by VM1--VM2. Then VM1-VM2-VM3-VM4. Then VM1-VM2-VM3-VM4-VM6-VM7-VM8-VM9-VM10
****************************************************:USER DOCUMENTATION:**************************************************
In our assignment development we tried to module different functionality into different file. Following are the files used in an integrated and synchronised way to achieve the final output:

server.c – The prime file responsible for initiating server, checking for clients requests and serving them by sending server local time.
client.c - The prime file responsible for initiating client, initialising communication with server and request for server local time.
odr.c - The prime file for implementing On-Demand shortest-hop Routing (ODR) protocol for routing messages in networks from one virtual machines to another and between server and client.
util.c – It’s the utility file used by client and server and odr for API that enables applications to communicate with the ODR mechanism running locally at their nodes using Unix domain datagram sockets.
get_hw_addrs.c – An API for discovering underline connected network interfaces information.

To compile the program, all of these files should be in the same folder with Makefile. The command used is:
$make

It will compile all these files and generate executives for all of them.

To run the program, first run the server program by the following command:
$./server

To run the ODR programm,provide staleness as a argument or the default staleness will be taken:

$./odr 6

Secondly, to run the client program, from a separate terminal use the following command:
$./client


Design Details:

Packet Structure :
_____________________
|Packet Type	     | 
|____________________|
|Broadcast ID	     | 
|____________________|
|Source Port|        |
|____________________|
|Destination Port    |
|____________________|
|source IP           | 
|____________________| 
|Destination IP      |
|____________________|
|Hops to destination |
|____________________|
|Payload             |  
|____________________|
|Force Docrovery Flag|
|____________________|
|rrep_sent           |
|____________________|

PF Frame Structure:
 __________________________
|Destination MAC Address   |
|__________________________|
|source MAC Address        |
|__________________________|
|Protocol NUMBER           |
|__________________________|	----unique protocol number used for our group is :0x4325
|Packet                    |  
|__________________________|


Routing Table Structure :

 _________________________
|Socket if_index          |	----
|_________________________|
|Valid Flag               |	----Flag to keep track of if a routing table entry is valid or not.
|_________________________|
|Next Hop                 | ----Next Hop from a particular VM.
|_________________________|
|Hops to Destination      |	----Total Hop Count till destination
|_________________________|
|Time Stamp               |	----Time stamp when the routing entry has created or last updated.
|_________________________|
|Broadcast Number         |	----Broadcart Number
|_________________________|
|Packet Queue             |	----Waiting packets in a particular VM.
|_________________________|

***************************************************:SYSTEM DOCUMENTATION:****************************************************

Features Implemented :


The API: To communicate using UNIX domain socket between ODR and client or Server two API's msg_send() and msg_recv() has been inplemented.

Client: Clients works in an infinite loop by asking user to input server "VM" number and connects to its ODR to send request to server and waits for server to respond back for certain time. If server does not respond back in defined "CLIENT_TIMEOUT" time, client retransmit request for single time and wait for response. To exit the client type "quit" or wait for retrasmitted request to time out.

Server: Server waits for clients requests and when receives a request serve with sending local time to different clients through its local ODR. 

ODR:
1)A staleness parameter is received from user to maintain the validity of a routing table enrty. A default staleness aslo has been taken care of in case use does not provide slate time.A routing table entry which is stale should be updated using RREQ.

1)It prints the index, and associated (unicast) IP and Ethernet addresses for each of the node’s interfaces using get_hw_addrs() and stores them in a structure of IfaceInfo for referring throughout the program.

2)Creates UNIX domain socket to communicate with own servoweer/ client and PF Sockets to communicate with other ODR program by determining the kernel provided index of the interface on which the incoming frame receives.

3)Builds and maintain Routing table for keeping track of shortest path between nodes using updateRoutingTable().

4)Whenever ODR receives a RREQ packet if floods to all interfacses connected using processRREQ() and floodPacket() if there is no path available in routing table for destination or the path available of stale. IF RREQ is looped back to its source then it is discarded in processRREQ().

5) ODR generates RREP messages if it is at the destination node, or if it is at an intermediate node that happens to have a route not stale, to the destination.It has been implemented using processRREP() and sendPacketToInterface() to send RREP packets to source. At the same time, a ‘forward’ path to the destination is entered into the routing table.

6) Application payload messages also used to update routing tables through processDATA() and updateRoutingTable() to keep track of latest and shortest route.

****************************************************:TESTING DOCUMENTATION:**************************************************
To test the application properly user can use the following commands:
INPUT	:	$./server
EXPECTED OUTPUT	:	Server program should start and print its VM's information and waits for client requests. Once request arrives it should response with server time.

INPUT	:	$./odr 6
EXPECTED OUTPUT	: ODR program will start by printing all the interface information and having a stalenes time period of "6".

INPUT	:	$./client
EXPECTED OUTPUT	:	Client program should start and prints its VM's information and send request to its ODR for time and waits till server replies back.

On client input ,provide VM values as "1" "2" or "vm1" or "vm2" to get time from server
Program works for most of cases. However in case when full topology is up, it fails to communicate with VM6 and connected VMs
It also might crash in some cases. However it works correctly for partial topology. For ex VM1-VM4-VM6-VM9-VM10.
Only when VM5 is enabled, it seems to face some problems




