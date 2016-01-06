#include "odr.h"

//int sendwaitingpackets(routing_entry *routingTable,IfaceInfo *iFaceDataList,int dest);
FilePortTable fpTable;
char odrVMIP[100];
struct hwa_info *hwahead;
static int nextBroadcastID = FIRST_BCAST_ID;
int global_raw_sock;
int map_hw_to_iface[10];
routing_entry routes[11];
void DisplayRoutingTable(routing_entry *routingTable,IfaceInfo *iFaceDataList, int rowNo);
void write_odr_packet(packet *packet_odr,int type,uint32_t broadcast_id,uint32_t srcPort,uint32_t destPort,char *destip,uint32_t hops,char *payload,uint32_t flag_force,uint32_t rrep_sent);
void readDomainSock(int domainSockFd, char *data, char *destIP, int *destPort, char *tempFile,int *flag)
{
	
	pckData aPacket;
	socklen_t pckLen, destAddrLen;
	struct sockaddr_un destAddr;
	
	destAddrLen=sizeof(destAddr);
	pckLen=sizeof(aPacket);
	Recvfrom(domainSockFd, &aPacket, pckLen, 0,  (struct sockaddr*)&destAddr, &destAddrLen);
	printf("\nrecved pcket");
	strcpy(tempFile,destAddr.sun_path);
	memcpy(data,aPacket.data,strlen(aPacket.data));
	data[strlen(data)]='\0';
	memcpy(destIP,aPacket.canonicalIP,strlen(aPacket.canonicalIP));
	destIP[strlen(destIP)]='\0';
	*destPort=aPacket.port;
	
	printf("\nMessage from server:%s, %s",data,aPacket.data);
	
}

void initFilePortMap()
{
			getTempPath(fpTable.fpMap[0].filePath,SERV_TEMP_FILE,0);
			fpTable.fpMap[0].portNo = SERVER_PORT;
			fpTable.fpMap[0].timeStamp = time(NULL);
			fpTable.fpMap[0].isValid = 1;
			fpTable.totalFilePortMap=1;
			fpTable.nextPortNo=INIT_CLI_PORTNO;

}
int getPortByFilePath(char *tempFile)
{
	int i;
	for(i=0;i<fpTable.totalFilePortMap;i++)
	{
		if(fpTable.fpMap[i].isValid)
		{
			if(difftime(time(NULL),fpTable.fpMap[i].timeStamp)<FP_MAP_STALE_VAL)
			{
				if(strcmp(fpTable.fpMap[i].filePath,tempFile)==0)
				{
					return fpTable.fpMap[i].portNo;
				}
			}
			else
			{
				fpTable.fpMap[0].isValid=0;
			}
		}
	}
	
	strcpy(fpTable.fpMap[i].filePath,tempFile);
    fpTable.fpMap[i].portNo = fpTable.nextPortNo++;
    fpTable.fpMap[i].timeStamp = time(NULL);
    fpTable.fpMap[i].isValid = 1;
    if (i == fpTable.totalFilePortMap) 
		fpTable.totalFilePortMap++;
	
	return fpTable.fpMap[i].portNo;
}

void getFilePathFromPort(char *filePath, int port)
{
	int i;
	for(i=0;i<fpTable.totalFilePortMap;i++)
	{
		if(fpTable.fpMap[i].portNo==port)
		{
			strcpy(filePath,fpTable.fpMap[i].filePath);
			return;
		}
	}
	
	printf("\nFileName could not be found!");
}
int writeDomainSock(packet *packet_odr,int domainSockFd, char *data, char *destIP, int destPort, int srcPort)
{
	
	pckData aPacket;
	socklen_t destAddrLen, pckLen;
	struct sockaddr_un destAddr;
	char filePath[521];
	char payload[512];
	
	aPacket.port=srcPort;
	aPacket.flag=0;
	memset(aPacket.data,0,sizeof(pckData));
	memcpy((void *)aPacket.data,(void *)data,strlen(data));
	aPacket.data[strlen(aPacket.data)]='\0';
	memcpy(aPacket.canonicalIP,destIP,strlen(destIP));
	aPacket.canonicalIP[strlen(aPacket.canonicalIP)]='\0';
	
	bzero(&destAddr,sizeof(destAddr));
	destAddr.sun_family=AF_LOCAL;
	getFilePathFromPort(filePath,destPort);
	strcpy(destAddr.sun_path,filePath);
	printf("path received:%s",filePath);
	
	destAddrLen=sizeof(destAddr);
	pckLen=sizeof(aPacket);
	int i;
	strcpy(payload,"\n Server Down!");
	
	if((i=sendto(domainSockFd, &aPacket, pckLen, 0, (struct sockaddr*)&destAddr, destAddrLen))<0)
	{
		write_odr_packet(packet_odr,DATA,0,SERVER_PORT,INIT_CLI_PORTNO,aPacket.canonicalIP,0,payload,0,0);
		printf("\nafter error!");
		return 0;
	}
	return 1;
	
}

void write_odr_packet(packet *packet_odr,int type,uint32_t broadcast_id,uint32_t srcPort,uint32_t destPort,char *destip,uint32_t hops,char *payload,uint32_t flag_force,uint32_t rrep_sent)
{   memset(packet_odr,'\0',sizeof(packet));
    packet_odr->pak_type = type;
    packet_odr->broadcast_id = broadcast_id;
    strcpy(packet_odr->source_ip, odrVMIP);
    strcpy(packet_odr->dest_ip, destip);
    packet_odr->source_port = srcPort;
    packet_odr->dest_port = destPort;
    packet_odr->flag_force = flag_force;
    packet_odr->hops =hops;
    packet_odr->rrep_sent = rrep_sent;
    bzero(packet_odr->payload,512);
    memcpy((void *)packet_odr->payload, (void *)payload,512);
	printf("\nInside write_odr_packet data:%s ",packet_odr->payload);

}
int isRoutable(int domainSockFd,packet *packet_odr)
{
	char data[512],destIP[512],tempFile[512];
	int destPort,srcPort,flag;
	readDomainSock(domainSockFd, data, destIP, &destPort, tempFile,&flag);
	srcPort=getPortByFilePath(tempFile);
	
	//writeDomainSock(domainSockFd, data, destIP, destPort, srcPort);
	int flag_local = strcmp(destIP,odrVMIP);
	if (flag_local == 0) 
	{

        writeDomainSock(packet_odr,domainSockFd, data, destIP, destPort, srcPort);
		return 0;
	} 
	else 
	{
		// we have set flag_florce to 0, check out for possible bug
		write_odr_packet(packet_odr,DATA,0,srcPort,destPort,destIP,1,data,0,0);
		return 1;

	}

}
void printHWAdr(struct hwa_info *hwa)
{
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag;
	
	printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\n interface i = %d\n\n", hwa->if_index);
}



int createIfaceSockets(IfaceInfo **ifaceSockList, fd_set *fdSet) {
    struct hwa_info *hwa ;
    int totalInterfaces = 0, i = 0,ethernetSockFd;
    struct sockaddr_ll *bind_sock = Malloc(sizeof(struct sockaddr_ll));
  //  struct sockaddr_ll readingSock;

    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next, totalInterfaces++);

    *ifaceSockList = Malloc(totalInterfaces * sizeof(IfaceInfo));

  /*  bzero(&readingSock, sizeof(readingSock));

    readingSock.sll_family   = PF_PACKET;    
    readingSock.sll_protocol = htons(PROTOCOL_NUMBER);  
	readingSock.sll_hatype   = ARPHRD_ETHER;
	readingSock.sll_pkttype  = PACKET_OTHERHOST;
	readingSock.sll_halen    = ETH_ALEN;
*/
		ethernetSockFd = socket(AF_PACKET, SOCK_RAW, htons(PROTOCOL_NUMBER));
		global_raw_sock = ethernetSockFd;
		bind_sock->sll_family = AF_PACKET;
		bind_sock->sll_protocol = htons(PROTOCOL_NUMBER);

    for (hwa = hwahead; hwa != NULL; hwa = hwa->hwa_next) {
        printHWAdr(hwa);

        if ((strcmp(hwa->if_name, "lo") != 0) && (strcmp(hwa->if_name, "eth0") != 0)) {
            // if the interface number is greater than 2 then make sockets on each interfaces
           /* if ((((*ifaceSockList)[i]).ifaceSocket = socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_NUMBER))) < 0) {
                err_quit("Error in creating PF_PACKET socket for interface: %d", i + 3);
            }*/
          //  readingSock.sll_ifindex = hwa->if_index;
           // memcpy(readingSock.sll_addr, hwa->if_haddr, MACLEN);

            ((*ifaceSockList)[i]).ifaceNum = hwa->if_index;
            memcpy(((*ifaceSockList)[i]).ifaceMAC, hwa->if_haddr, MACLEN);
		((*ifaceSockList)[i]).ifaceSocket = ethernetSockFd;
		map_hw_to_iface[hwa->if_index] = i;
		//int sock_fd =1;
            //FD_SET(ethernetSockFd,fdSet);
            //Bind((*ifaceSockList)[i].ifaceSocket, (SA *) &readingSock, sizeof(readingSock));
            i++;
        }
    }
    
    free_hwa_info(hwahead);
    printf("\n%d interfaces Bind\n", i);
    return i;
}
void ethoNtoP(char *addr,char *result)
{
        char buf[10];
        char *temp ;
        temp = result;
    int i;
    temp[0] = '\0';
    for (i = 0; i < 6; i++) {

        sprintf(buf, "%.2x%s", addr[i] & 0xff , i == 5 ? "" : ":");

        strcat(temp, buf);
    }
     int len = strlen(temp);
     temp[len] = '\0';

}

void printiFacePacket(Frame *frame)
{
	packet *ODRPck;
	ODRPck=&(frame->packet_odr);
	
	
        char dest_mac[30];
        char source_mac[30];
        ethoNtoP(frame->source_mac,source_mac);
        ethoNtoP(frame->dest_mac,dest_mac);
	int proto = frame->proto_id;
	char *srcIP = ODRPck->source_ip;
	char *destIP = ODRPck->dest_ip;
	int srcPort = ODRPck->source_port;
	int dstPort = ODRPck->dest_port;
	int hop_count = ODRPck->hops;
	int b_id = ODRPck->broadcast_id;
	int type = ODRPck->pak_type;

        printf(" \nEthernet frame header: Source Mac: %s , Dest Mac : %s , ProtNum: %x ", source_mac,dest_mac,proto);	
	
     printf ("\n ODR packet header : Type %d , Src IP %s, Dest IP %s , Src Port %d ,Dst Port %d , Hop Count %d , Broadcast id %d",type,srcIP,destIP,srcPort,dstPort,hop_count,b_id);

    if (ODRPck->rrep_sent==1)
        printf("RREP sent ");
    else
        printf(" RREP  not sent ");

    if (ODRPck->flag_force==1)
        printf(" Force Discovery is ON ");
    else
        printf("Force Discovery is OFF ");

    printf("Data : %s \n", ODRPck->payload);
	

}

int isStale(routing_entry *route) {
    double diff = difftime(time(NULL), route->timestamp);
    return (diff >= (double)STALE_TIME) ? 1 : 0;
}

int isNewerOrShorterRoute(routing_entry *aRow,packet *ODRPck)
{
	
	//return 1=if updated, 0= if no update, 
	
    // check if any route is present
    if (aRow->valid == 0)
        return 1;

    // check if Route is stale
    if (isStale(aRow))
        return 1;

    // check if force discovery is required
    if (ODRPck->flag_force==0)
        return 0;
	
	//check if newer or older RREQ packet
	uint32_t newBroadcastID  = ODRPck->broadcast_id;
    if (aRow->broadcast_id != 0 && newBroadcastID != 0) 
	{
        if (aRow->broadcast_id < newBroadcastID)
            return 1;
        if (aRow->broadcast_id > newBroadcastID)
            return 0;
    }

    // A better hop count exists
	uint32_t newHopCnt = ODRPck->hops;
    if (aRow->hops > newHopCnt)
        return 1;
    // Another path with same hop count
    else if (aRow->hops == newHopCnt)
        return 0;

    
   return 0; //No better route found
}

void sendEthernetFrame(packet *packet_odr, uint8_t dstMAC[MACLEN], IfaceInfo *iFaceDataList,int index, int isBroadcast,int totalNoOfPFSock)
{
	 struct sockaddr_ll sockAddr;
	Frame frame;

    bzero(&sockAddr, sizeof(sockAddr));
    bzero(&frame, sizeof(frame));
    memset(&sockAddr,'\0',sizeof(sockAddr));
    memset(&frame,'\0',sizeof(frame));
	int i;
	/*void* buffer = (void*)malloc(ETH_FRAME_LEN);
	
	unsigned char* etherhead = buffer;
	

unsigned char* data = buffer + 14;
	

struct ethhdr *eh = (struct ethhdr *)etherhead;
 
int send_result = 0;


//unsigned char src_mac[6] = {0x00, 0x01, 0x02, 0xFA, 0x70, 0xAA};
unsigned char src_mac[6];

unsigned char dest_mac[6] = dstMAC;
*/

    /*RAW communication*/
    sockAddr.sll_family   = PF_PACKET;
    sockAddr.sll_protocol = htons(PROTOCOL_NUMBER);

    /*ARP hardware identifier is ethernet*/
    sockAddr.sll_hatype   = ARPHRD_ETHER;

    /*target is another host*/
    sockAddr.sll_pkttype  = PACKET_OTHERHOST;

    /*address length*/
    sockAddr.sll_halen    = ETH_ALEN;

    //memcpy(sockAddr.sll_addr,dstMAC, MACLEN);
    

    //memcpy(frame.source_mac, srcMAC, MACLEN);
    memcpy(frame.dest_mac, dstMAC, MACLEN);
    memcpy((void *)sockAddr.sll_addr,(void *)frame.dest_mac,MACLEN);
    memcpy(&(frame.packet_odr), packet_odr, sizeof(packet));
    frame.proto_id = htons(PROTOCOL_NUMBER);

    // Increment Hop count in the packet
    packet_odr->hops++;

	
	packet *aPacket;
    aPacket = &(frame.packet_odr);
	
	printf("ODR @ VM%d is sending ODR MSG of TYPE: %d  SRC: VM%d  DST: VM%d\n",
          getVMByIP(odrVMIP),aPacket->pak_type, getVMByIP(aPacket->source_ip), getVMByIP(aPacket->dest_ip));

		  
	if(isBroadcast==1)	  
	{
		for(i=0;i<totalNoOfPFSock;i++)
		{
			memcpy(frame.source_mac,iFaceDataList[i].ifaceMAC,MACLEN);
    		sockAddr.sll_ifindex  = (iFaceDataList[i]).ifaceNum;
			
			if (sendto(iFaceDataList[i].ifaceSocket, &frame, sizeof(Frame), 0, (SA*) &sockAddr, sizeof(sockAddr)) == -1)
			{
	
				printf("Error in sending Ethernet packet");
			}
			printiFacePacket(&frame);
		
		}
	}
	else
	{
		memcpy(frame.source_mac,iFaceDataList[index].ifaceMAC,MACLEN);
    	sockAddr.sll_ifindex  = (iFaceDataList[index]).ifaceNum;
			
			if (sendto(global_raw_sock, &frame, sizeof(Frame), 0, (SA*) &sockAddr, sizeof(sockAddr)) == -1)
			{
				printf("\n Error in sending Ethernet packet %s \n",strerror(errno));
			}
			printiFacePacket(&frame);
	}
    /* if (sendto(sfd, &frame, sizeof(Frame), 0, (SA*) &sockAddr, sizeof(sockAddr)) == -1)
	{
        printf("Error in sending Ethernet packet");
    } */
	printiFacePacket(&frame); 
	
	
}
void sendPacketToInterface(packet *packet_odr, uint8_t srcMAC[MACLEN], uint8_t dstMAC[MACLEN],uint16_t if_index, int sfd)
{
	int retVal;

    /*target address*/
    struct sockaddr_ll sockAddr;
	Frame frame;

    bzero(&sockAddr, sizeof(sockAddr));
    bzero(&frame, sizeof(frame));

    /*RAW communication*/
    sockAddr.sll_family   = PF_PACKET;
    sockAddr.sll_protocol = htons(PROTOCOL_NUMBER);

    /*ARP hardware identifier is ethernet*/
    sockAddr.sll_hatype   = ARPHRD_ETHER;

    /*target is another host*/
    sockAddr.sll_pkttype  = PACKET_OTHERHOST;

    /*address length*/
    sockAddr.sll_halen    = ETH_ALEN;

    memcpy(sockAddr.sll_addr,dstMAC, MACLEN);
    sockAddr.sll_ifindex  = if_index;

    memcpy(frame.source_mac, srcMAC, MACLEN);
    memcpy(frame.dest_mac, dstMAC, MACLEN);
    memcpy(&(frame.packet_odr), packet_odr, sizeof(packet));
    frame.proto_id = htons(PROTOCOL_NUMBER);

    // Increment Hop count in the packet
    packet_odr->hops++;

    
	packet *aPacket;
    aPacket = &(frame.packet_odr);
    printf("ODR @ VM%d is sending ODR MSG of TYPE: %d  SRC: VM%d  DST: VM%d\n",
          getVMByIP(odrVMIP),aPacket->pak_type, getVMByIP(aPacket->source_ip), getVMByIP(aPacket->dest_ip));

    if (sendto(sfd, &frame, sizeof(Frame), 0, (SA*) &sockAddr, sizeof(sockAddr)) == -1)
	{
        printf("Error in sending Ethernet packet");
    }
	printiFacePacket(&frame);
}
 int sendwaitingpackets(routing_entry *routingTable,IfaceInfo *iFaceDataList,int dest,int hw_if[])
{
	packet_list *waitingPackets;
    packet_list *freePacket;
    uint8_t srcMAC[MACLEN], dstMAC[MACLEN];
    uint16_t i, j;
    int sfd;
    int packetSent = 0;
    if(routingTable[dest].valid ==0) {
		return 0;
    }
		
    i = routingTable[dest].if_index;
    j = iFaceDataList[i].ifaceNum;
    sfd = iFaceDataList[i].ifaceSocket;
    int index = map_hw_to_iface[i];

    waitingPackets = routingTable[dest].packhead;
    freePacket = routingTable[dest].packhead;
    memcpy((void *)dstMAC, (void *)routingTable[dest].next_hop, MACLEN);
    memcpy((void *)srcMAC, (void *)iFaceDataList[i].ifaceMAC, MACLEN);
    if (waitingPackets == NULL)
	printf("no waiting packets \n"); 
    while (waitingPackets != NULL) {
	
        printf("Sent a waiting Packet of Type: %d\n", waitingPackets->packet_odr.pak_type);
	
	DisplayRoutingTable(routingTable,iFaceDataList,0);
        //sendEthernetFrame(&RREPPck,dataRow->next_hop,iFaceDataList,index, 0,totalNoOfPFSock);
	sendEthernetFrame(&(waitingPackets->packet_odr),dstMAC,iFaceDataList,index,0,0);
        waitingPackets = waitingPackets->next;
        packetSent++;

        free(freePacket);
        freePacket = waitingPackets;
    }
    routingTable[dest].packhead = NULL;
    return 1;
} 


void DisplayRoutingTable(routing_entry *routingTable,IfaceInfo *iFaceDataList, int rowNo)
{   int dest_vm = rowNo;
    int valid = routingTable[rowNo].valid;
    int b_id = routingTable[rowNo].broadcast_id;
    char timestamp[60];
    strcpy(timestamp,asctime(localtime((const time_t *)&routingTable[rowNo].timestamp)));
    char str[25];
    int i;
    int int_num = iFaceDataList[routingTable[rowNo].if_index].ifaceNum;

   char next_hop[30];
   ethoNtoP(routingTable[rowNo].next_hop,next_hop); 
   int no_of_hops = routingTable[rowNo].hops;
   
    printf("\n*****Routing Table******");

    if (rowNo != 0) 
	{
	printf("\n Dest VM %d",dest_vm);
	printf(" Mac Next %s , Interface num %d",next_hop,int_num);
	printf(" Hops %d , Broadcast ID %d , Timestamp %s",no_of_hops,b_id,timestamp);
    } 
	else 
	{
	
        for (i = 1; i <= 10; i++)
		{

		int dest_vm = i;
    		int valid = routingTable[i].valid;
    		int b_id = routingTable[i].broadcast_id;
		strcpy(timestamp,asctime(localtime((const time_t *)&routingTable[i].timestamp)));
	int int_num = iFaceDataList[routingTable[i].if_index].ifaceNum;

   	ethoNtoP(routingTable[i].next_hop,next_hop);         
   	int no_of_hops = routingTable[i].hops;

            if (routingTable[i].valid) {
	        printf("\n Dest VM %d",dest_vm);
        	printf(" Mac Next %s , Interface num %d",next_hop,int_num);
                printf(" Hops %d , Broadcast ID %d , Timestamp %s",no_of_hops,b_id,timestamp);
	}
        }
    }
   printf("\n**************************************************\n");
}


static int updateRoutingTable(Frame *frame_ethernet, routing_entry *routingTable,IfaceInfo *iFaceDataList,int index)
{
	routing_entry *dataRow;
	packet *ODRPck;
	int source,isRouteUpdated;;
	ODRPck=&(frame_ethernet->packet_odr);
	
    source= getVMByIP(ODRPck->source_ip);
    dataRow=&routingTable[source];
    //memcpy((void *)dataRow->next_hop,(void *)frame_ethernet->source_mac, MACLEN); 

    isRouteUpdated = isNewerOrShorterRoute(dataRow, ODRPck);
	//int packetsSent  = sendwaitingpackets(routingTable,iFaceDataList,source);
    if (isRouteUpdated != 0) 
	{

	
        int packetsSent;
	memcpy(dataRow->next_hop, frame_ethernet->source_mac, MACLEN);
	dataRow->timestamp = time(NULL);
	memcpy((void *)dataRow->next_hop,(void *)frame_ethernet->source_mac, MACLEN);
        dataRow->valid = 1;
        dataRow->broadcast_id = ODRPck->broadcast_id;
        dataRow->if_index = iFaceDataList[index].ifaceNum;
        dataRow->hops = ODRPck->hops;
	
	packetsSent  = sendwaitingpackets(routingTable,iFaceDataList,source,map_hw_to_iface);

    }
    return isRouteUpdated;
	
}

/* void setODRPacket(packet *packet, int type, char *srcIP, char *dstIP,
                  uint32_t srcPort, uint32_t dstPort, int hopCnt, int broadID,
                  int isRREP_sent, int forceDiscovery, char* data, int length)
{   memset(packet,'\0',sizeof(packet));
    packet->pak_type = type;
	packet->broadcast_id = broadID;
	packet->source_port = srcPort;
	packet->dest_port = dstPort;
    memcpy(packet->source_ip, srcIP, IPLEN);
    memcpy(packet->dest_ip, dstIP, IPLEN);
    packet->hops = hopCnt;
    memcpy(packet->payload, data, length);
	packet->flag_force = forceDiscovery;
    packet->rrep_sent = isRREP_sent;
    
    
} */

void  addPacketToList(packet *packet_in, routing_entry *route, int dest)
{
	packet_list *newPck=(packet_list*)Malloc(sizeof(packet_list));
	packet *newp = &newPck->packet_odr;
	memcpy((void *)newp,(void *)packet_in,sizeof(packet));
	newPck->next=route[dest].packhead;
	route[dest].packhead=newPck;
}


int isRouteAvailable(packet *packet, routing_entry *route) {
    int dest = getVMByIP(packet->dest_ip);
    routing_entry *aRoute = &(route[dest]);

    if ((aRoute->valid == 0) || isStale(aRoute))           
    {
		printf("Route not present");
		aRoute->valid = 0;
        if (packet->pak_type != RREQ) 
			addPacketToList(packet, route, dest);
        
        return 0;
    }
    return 1;
}
void floodPacket(packet *apacket, IfaceInfo *iFaceDataList,int index, int totalNoOfPFSock)
{
	
    uint8_t broadcastMACAdd[MACLEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	sendEthernetFrame(apacket, broadcastMACAdd,iFaceDataList,index, 1,totalNoOfPFSock);
	/*int retVal,i;
	//struct sockaddr_ll sockAddr;
    Frame frame;

	bzero(&frame, sizeof(Frame));
    bzero(&sockAddr, sizeof(struct sockaddr_ll));
    

    
    sockAddr.sll_family   = PF_PACKET;
    sockAddr.sll_protocol = htons(PROTOCOL_NUMBER);

    
    sockAddr.sll_hatype   = ARPHRD_ETHER;


    sockAddr.sll_pkttype  = PACKET_OTHERHOST;


    sockAddr.sll_halen    = ETH_ALEN;
    memcpy(sockAddr.sll_addr, broadcastMACAdd, MA/moniCLEN);

    memcpy(frame.dest_mac, broadcastMACAdd, MACLEN);
    memcpy(&(frame.packet_odr), apacket, sizeof(packet));
    frame.proto_id = htons(PROTOCOL_NUMBER);
	apacket->hops++;
	
	
	for (hwa = hwahead; hwa != NULL; hwa = hwa->hwa_next) {
        printHWAdr(hwa);

        if ((strcmp(hwa->if_name, "lo") != 0) && (strcmp(hwa->if_name, "eth0") != 0)) {
            // if the interface number is greater than 2 then make sockets on each interfaces
           /* if ((((*ifaceSockList)[i]).ifaceSocket = socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_NUMBER))) < 0) {
                err_quit("Error in creating PF_PACKET socket for interface: %d", i + 3);
            }
            readingSock.sll_ifindex = hwa->if_index;
            memcpy(readingSock.sll_addr, hwa->if_haddr, MACLEN);

            ((*ifaceSockList)[i]).ifaceNum = hwa->if_index;
            memcpy(((*ifaceSockList)[i]).ifaceMAC, hwa->if_haddr, MACLEN);
            FD_SET((*ifaceSockList)[i].ifaceSocket, fdSet);
            //Bind((*ifaceSockList)[i].ifaceSocket, (SA *) &readingSock, sizeof(readingSock));
            i++;
        }
    }
    for (i = 0; i < totalNoOfPFSock; i++) {

        if (i != index) {

            memcpy(frame.source_mac, iFaceDataList[i].ifaceMAC, MACLEN);
            sockAddr.sll_ifindex  = iFaceDataList[i].ifaceNum;

            printf("\nFlooding RREQ Packet on interface number: %d\n",iFaceDataList[i].ifaceNum);

			packet *newPck;		
			newPck = &(frame.packet_odr);
			printf("ODR @ VM%d is sending ODR MSG of TYPE: %d  SRC: VM%d  DST: VM%d\n",
			getVMByIP(odrVMIP),newPck->pak_type, getVMByIP(newPck->source_ip), getVMByIP(newPck->dest_ip));

			if (sendto(iFaceDataList[i].ifaceSocket, &frame, sizeof(Frame), 0, (SA*) &sockAddr, sizeof(sockAddr)) == -1)
			{
				printf("Error in sending Ethernet packet");
			}
			printiFacePacket(&frame);
        }
    }
*/
	

}

void processRREQ(Frame *frame, routing_entry *routingTable, IfaceInfo *iFaceDataList, int totalNoOfPFSock,int index)
{
	uint32_t  destIndex;
    packet *apacket;
    int nwdest,isRouteUpdated;
      routing_entry *dataRow;
        packet *ODRPck;
        int source;
        ODRPck=&(frame->packet_odr);

    source= getVMByIP(ODRPck->source_ip);
    dataRow=&routingTable[source];
 
    

    apacket = Malloc(sizeof(packet));
    memcpy(apacket, &(frame->packet_odr), sizeof(packet));
	
	// Do nothing when RREQ received from original source, stop flooding.
    if (strcmp(apacket->source_ip, odrVMIP) ==0) 
	{
		printf("\n RREQ is looped back !\n");
        return;
    }
    if (apacket->hops == TTL_HOP_COUNT) 
	{
        printf("RREQ is ignored as hop count(%d) is equal to TTL\n", apacket->hops);
        return;
    }

    isRouteUpdated = updateRoutingTable(frame,routingTable,iFaceDataList,index);

    // Only send RREPs if Already Sent flag "Asent" is FALSE
    if (!apacket->rrep_sent) 
	{
		packet RREPPck;
		int srcVm = getVMByIP(apacket->source_ip);
        int destVm = getVMByIP(apacket->dest_ip);
        if (strcmp(apacket->dest_ip, odrVMIP) == 0) 
		{
            if (isRouteUpdated == 1) 
			{
                // Create RREPs and send them back only for better/fresher RREQ packets
                packet RREPPck;
                nwdest = getVMByIP(apacket->source_ip);

                setODRPacket(&RREPPck, RREP, apacket->dest_ip, apacket->source_ip,
                        apacket->dest_port, apacket->source_port, 1, 0,0,
                        apacket->flag_force, NULL, 0);

                printf("\nRREP Packet Sent\n");
		DisplayRoutingTable(routingTable,iFaceDataList,0);
		sendEthernetFrame(&RREPPck,dataRow->next_hop,iFaceDataList,index, 0,totalNoOfPFSock);
		//		sendPacketToInterface(&RREPPck, iFaceDataList[index].ifaceMAC, routingTable[nwdest].next_hop,iFaceDataList[index].ifaceNum, iFaceDataList[index].ifaceSocket);
               
            }
            return;
        }

        if (apacket->flag_force==1) 
		{
            routingTable[destVm].valid = 0;
		} 
		else if (isRouteAvailable(apacket, routingTable)==1) {
            // Create RREPs and send them back
            setODRPacket(&RREPPck, RREP, apacket->dest_ip, apacket->source_ip,
                        apacket->dest_port, apacket->source_port, routingTable[srcVm].hops + 1, 0,0,
                        apacket->flag_force, NULL, 0);
           
			printf("\nRREP Packet Sent\n");
			sendPacketToInterface(&RREPPck, iFaceDataList[index].ifaceMAC, routingTable[nwdest].next_hop,routingTable[nwdest].if_index, iFaceDataList[index].ifaceSocket);
			apacket->rrep_sent = 1;
        }
    }

    // Route not present or updated, so keep flooding RREQ
    if (isRouteUpdated != 0) 
	{
        apacket->hops++;
        floodPacket(apacket, iFaceDataList, index, totalNoOfPFSock);
    }
}

void processRREP(Frame *frame,routing_entry *routingTable,IfaceInfo *iFaceDataList,int totalNoOfPFSock,int index)
{
	
    uint32_t  destIndex;
    packet *apacket;
    int nwdest,isRouteUpdated;
    
    
    
    apacket = Malloc(sizeof(packet));
    memcpy(apacket, &(frame->packet_odr), sizeof(packet));

    
	isRouteUpdated = updateRoutingTable(frame,routingTable,iFaceDataList,index);
    if ((isRouteUpdated != 1) || (strcmp(apacket->dest_ip, odrVMIP) == 0)) 
	{
        
        return;
    }

    if (isRouteAvailable(apacket, routingTable)==1) 
	{
        // Send RREP to source
		nwdest = getVMByIP(apacket->dest_ip);
        destIndex = routingTable[nwdest].if_index;
	int new_index = map_hw_to_iface[destIndex]; 
        apacket->hops++;
        printf("RREP Packet sent\n");
		sendPacketToInterface(apacket, iFaceDataList[new_index].ifaceMAC, routingTable[nwdest].next_hop,destIndex, iFaceDataList[new_index].ifaceSocket);
        
    } 
	else 
	{

        printf("\nRoute is not present in Routing Table , RREQ is created\n");

        packet RREQPck;
		nextBroadcastID++;
		setODRPacket(&RREQPck, RREQ,odrVMIP, apacket->dest_ip,0 ,
                        apacket->dest_port, 1, nextBroadcastID,0,
                        apacket->flag_force, NULL, 0);
        

        floodPacket(&RREQPck, iFaceDataList, index, totalNoOfPFSock);
	
    }
	
}

void processDATA(Frame *frame, routing_entry *routingTable, IfaceInfo *iFaceDataList, int totalNoOfPFSock,int index,int domainSockfd)
{
	
    packet *apacket;
    packet err_packet;
    
    apacket = Malloc(sizeof(packet));
    memset((void *)apacket,'\0',sizeof(packet));
    memcpy((void *)apacket, (void *)&(frame->packet_odr), sizeof(packet));

    uint32_t  destIndex;
    int nwdest = getVMByIP(apacket->dest_ip);
    int newSrc;

    printf("\n Packet type - DATA: Vm%d\n", getVMByIP(apacket->source_ip));
    updateRoutingTable(frame,routingTable,iFaceDataList,index);
   	
        uint8_t dest_mac[6];
        uint8_t src_mac[6];
 
	
    if (strncmp(apacket->dest_ip, odrVMIP,IPLEN) == 0) 
	{
        printf("Sending DATA to %s:%d \n", apacket->source_ip, apacket->source_port);
		int success;
		success=writeDomainSock(&err_packet,domainSockfd, apacket->payload, apacket->source_ip, apacket->dest_port, apacket->source_port);
		if(success==0)
			handle_client_request(iFaceDataList,totalNoOfPFSock,&err_packet,routingTable);
        return;
    }

    if (isRouteAvailable(apacket, routingTable)==1) 
	{
                int dest_index = getVMByIP(apacket->dest_ip);
                int if_index   = routingTable[dest_index].if_index;
                int new_index = map_hw_to_iface[if_index];
                int if_num     = iFaceDataList[new_index].ifaceNum;
                int sock_num   = iFaceDataList[new_index].ifaceSocket;

                apacket->flag_force = 0;
		apacket->hops++;

                memcpy(dest_mac,routingTable[dest_index].next_hop,6);
                memcpy(src_mac,iFaceDataList[if_index].ifaceMAC,6);
                sendPacketToInterface(apacket,src_mac,dest_mac,if_num,sock_num);

        //destIndex = routingTable[nwdest].if_index;
	
        //apacket->hops++;
        
		//sendPacketToInterface(apacket, iFaceDataList[index].ifaceMAC, routingTable[nwdest].next_hop,destIndex, iFaceDataList[index].ifaceSocket);
        
    } 
	else
	{
        packet DATAPck;
		
	setODRPacket(&DATAPck, RREQ,odrVMIP, apacket->dest_ip,apacket->source_port ,
                        apacket->dest_port, 1, nextBroadcastID,0,
                        apacket->flag_force, NULL, 0);
       

        floodPacket(&DATAPck, iFaceDataList, index, totalNoOfPFSock);
    }
	
}




void processiFacePacket(Frame *frame, int domainSockfd, IfaceInfo *iFaceDataList,int totalNoOfPFSock, int index,routing_entry *routingTable)
{
	
	packet *ODRPck;
	ODRPck=&(frame->packet_odr);
	
	switch (ODRPck->pak_type) 
	{
        case RREQ: // RREQ packet
			printf("\nRREQ received\n");
			processRREQ(frame, routingTable, iFaceDataList, totalNoOfPFSock,index);
			printf("\nRREQ processed succesfully \n");
            break;

        case RREP: // RREP packet
			printf("\nRREP received\n");
            processRREP(frame, routingTable, iFaceDataList, totalNoOfPFSock,index);
			printf("\nRREP processed succesfully \n");
            break;

        case DATA: // Data packet
			printf("\nData packet received!\n");
			processDATA(frame, routingTable, iFaceDataList, totalNoOfPFSock,index,domainSockfd);
			break;
	}
	
}

int handle_client_request(IfaceInfo *iFaceDataList,int totalNoofPFSock,packet *packet_odr, routing_entry *routingTable){

	int if_num,if_index;
	int dest_index,sock_num;
	uint8_t dest_mac[6];
	uint8_t src_mac[6];

	if (isRouteAvailable(packet_odr, routingTable)==1) 
	{
		dest_index = getVMByIP(packet_odr->dest_ip);
		if_index   = routingTable[dest_index].if_index;
		int new_index = map_hw_to_iface[if_index]; 
		if_num     = iFaceDataList[new_index].ifaceNum;
		sock_num   = iFaceDataList[new_index].ifaceSocket;
	
		packet_odr->flag_force = 0;
		
		memcpy(dest_mac,routingTable[dest_index].next_hop,6);
		memcpy(src_mac,iFaceDataList[if_index].ifaceMAC,6);
		sendPacketToInterface(packet_odr,src_mac,dest_mac,if_num,sock_num);
		
		return 0;
	}
	else 
	{
		packet RREQPck;
		nextBroadcastID++;
		setODRPacket(&RREQPck, RREQ,odrVMIP,packet_odr->dest_ip,
               packet_odr->source_port,packet_odr->dest_port, 0, nextBroadcastID,0,
                        1, NULL, 0);
					
		floodPacket(&RREQPck, iFaceDataList, -1, totalNoofPFSock);
		
		return 1;
	} 

}
void monitorSockets(int domainSockfd,IfaceInfo *iFaceDataList,fd_set fdSet,int totalNoOfPFSock)
{
		int maxFd,i;
		fd_set readingFD;
		packet packet_odr;
		routing_entry *routingTable;
		struct sockaddr_ll readingsock;
		
		routingTable=routes;
		//memset(routingTable,'0',sizeof(routes));
		maxFd=max(domainSockfd,global_raw_sock);
		/* for(i=0;i<totalNoOfPFSock;i++)
		{
			maxFd=max(maxFd,iFaceDataList[i].ifaceSocket);
		} */
		maxFd=maxFd+1;
		
		while(1)
		{
			readingFD=fdSet;
			select(maxFd,&readingFD,NULL,NULL,NULL);
			
			if(FD_ISSET(domainSockfd,&readingFD))
			{
				if(isRoutable(domainSockfd,&packet_odr)==1)
				{
					handle_client_request(iFaceDataList,totalNoOfPFSock,&packet_odr,routingTable);
				}
			}
			
			if(FD_ISSET(global_raw_sock,&readingFD))
				{       printf("reached select ");
					Frame frame;
					//bzero(frame, sizeof(Frame));
					memset(&readingsock,'\0',sizeof(struct sockaddr_ll));
					memset(&frame,'\0',sizeof(frame));
					int len;
					if (recvfrom(global_raw_sock, &frame, sizeof(Frame), 0, (SA *)&readingsock, &len) < 0) 
					{
						printf("Error in receiving Ethernet packet");
					}

					int i = map_hw_to_iface[readingsock.sll_ifindex];
					printf(" \n Packet received on interface %d \n",readingsock.sll_ifindex);
					printiFacePacket(&frame);
					
					processiFacePacket(&frame, domainSockfd, iFaceDataList,totalNoOfPFSock, i,routingTable );
				
				}		
					
			
		}
	
	
}
int main(int argc, char *argv[])
{
	int staleTime, odrVM;
	IfaceInfo *iFaceDataList;
	char filePath[1024];
	fd_set fdSet;
	int domainSockfd,totalNoOfPFSock;
	if(argc==1)
	{
		staleTime=6;
		printf("Staleness is missing ! Deafult staleness is captured!");
	}
	else
		staleTime=atoi(argv[1]);

	getTempPath(filePath,ODR_TEMP_FILE,0);
	
	
	//getTempPath(filePath,CLI_TEMP_FILE);
	odrVM=getHostVMNo();
	getVMsIP(odrVMIP);
	printf("\nODR is running on %s in VM %d ",odrVMIP,odrVM);
	
	initFilePortMap();
	FD_ZERO(&fdSet);
	
	domainSockfd=createDomaimDGramSock(filePath);
	FD_SET(domainSockfd, &fdSet);
	printf("\nODR interface addresses:");
	totalNoOfPFSock=createIfaceSockets(&iFaceDataList,&fdSet);
	FD_SET(global_raw_sock,&fdSet);
	
	monitorSockets(domainSockfd,iFaceDataList,fdSet,totalNoOfPFSock);
	
	free_hwa_info(hwahead);
	close(domainSockfd);
	
}	

