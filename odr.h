#ifndef _ODR_H
#define _ODR_H

#include "unp.h"
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include "utility.h"
#include "hw_addrs.h"
#include <errno.h>

#define RREQ 1
#define RREP 2
#define DATA 3

#define STALE_TIME	600000.0
#define PROTOCOL_NUMBER 0x4325
#define IPLEN           30      // bytes
#define MACLEN          6       // bytes
#define MAX_PAYLOAD_LEN 100     // bytes
#define FP_MAP_STALE_VAL 6.0    // sec
#define TTL_HOP_COUNT    10     // hops
#define INIT_CLI_PORTNO 4000
#define FIRST_BCAST_ID   8000

typedef struct {
    int ifaceNum;
    int ifaceSocket;
    uint8_t ifaceMAC[MACLEN];
} IfaceInfo;

typedef struct {
    char filePath[1024];
    int portNo;
    time_t timeStamp;
    int isValid;
} FilePortMap;

typedef struct {
	FilePortMap fpMap[100];
	int totalFilePortMap;
	int nextPortNo;
}FilePortTable;

typedef struct {
	uint32_t pak_type;
	uint32_t broadcast_id;
	uint32_t source_port;
	uint32_t dest_port;
	char     source_ip[30];
	char     dest_ip[30];
	uint32_t hops;
	char     payload[512];
	uint32_t flag_force; 
	uint32_t rrep_sent;
} packet;

typedef struct packet_list {
	struct packet_list *next;
	struct packet_list *previous;
	packet packet_odr;
} packet_list;
	
typedef struct {
	uint32_t if_index;
	uint32_t valid;
	uint8_t next_hop[6];
	uint32_t hops;
        time_t   timestamp;
	uint32_t broadcast_id;
	packet_list *packhead;	
} routing_entry;

typedef struct {
	uint8_t dest_mac[6];
        uint8_t source_mac[6];
        uint16_t proto_id;
        packet packet_odr;
} Frame;
int sendwaitingpackets(routing_entry *routingTable,IfaceInfo *iFaceDataList,int dest,int hw_array[]);
void sendEthernetFrame(packet *packet_odr, uint8_t dstMAC[MACLEN], IfaceInfo *iFaceDataList,int index, int isBroadcast,int totalNoOfPFSock);
void setODRPacket(packet *packet, int type, char *srcIP, char *dstIP,
                  uint32_t srcPort, uint32_t dstPort, int hopCnt, int broadID,
                  int isRREP_sent, int forceDiscovery, char* data, int length);

#endif
