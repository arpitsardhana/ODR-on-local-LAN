#include "utility.h"
#include "unp.h"
#include "odr.h"
void getTempPath(char *filePath,char *fileName,int flag)
{

        if((getcwd(filePath,1024))==NULL)
        {
                printf("\nRandom path can not be generated!");
        }
        printf("\n automatic path:%s",filePath);
        strcat(filePath,fileName);

	if (flag != 0) { 

        	if((mkstemp(filePath))==-1)
        	{
                	printf("\n No unique filename could be found!");
                	return;
        	}

	}


}
void msg_send(int sockfd, char *data, int flag,char *destIP, int destPort)
{
        pckData aPacket;
        socklen_t destAddrLen, pckLen;
        struct sockaddr_un destAddr;
        char filePath[256];
		int i;
		bzero(filePath,256);
		bzero(&destAddr,sizeof(destAddr));
		bzero(&aPacket.data,sizeof(PAYLOAD));
		
        aPacket.port=destPort;
        aPacket.flag=flag;
        memcpy(aPacket.data,data,strlen(data));
        aPacket.data[strlen(aPacket.data)]='\0';
        memcpy(aPacket.canonicalIP,destIP,strlen(destIP));
        aPacket.canonicalIP[strlen(aPacket.canonicalIP)]='\0';

        bzero(&destAddr,sizeof(destAddr));
        destAddr.sun_family=AF_LOCAL;
        
       
                getTempPath(filePath,ODR_TEMP_FILE,0);
                strcpy(destAddr.sun_path,filePath);
                printf("\n path received:%s",filePath);
        
        destAddrLen=sizeof(destAddr);
        pckLen=sizeof(aPacket);
        if((i=sendto(sockfd, &aPacket, pckLen, 0, (struct sockaddr*)&destAddr, destAddrLen))<0)
		{
			printf("\n Error in sendinf or Domain socket!");
		}
        printf("\nMSG sent");

}
void msg_recv(int sockfd, char *data, char *sourceIP, int *sourcePort)
{
        pckData aPacket;
        socklen_t pckLen, servAddrLen;
        struct sockaddr_un servAddr;

        servAddrLen=sizeof(servAddr);
        pckLen=sizeof(aPacket);
        Recvfrom(sockfd, &aPacket, pckLen, 0,  (struct sockaddr*)&servAddr, &servAddrLen);
        printf("\nrecved pcket \n ");
        memcpy(data,aPacket.data,strlen(aPacket.data));
        data[strlen(data)]='\0';
        memcpy(sourceIP,aPacket.canonicalIP,strlen(aPacket.canonicalIP));
        sourceIP[strlen(sourceIP)]='\0';
        *sourcePort=aPacket.port;

        //printf("\nMessage from server:%s, %s",data,aPacket.data);

}
void getVMsIP(char *hostIP)
{
        char hostName[200];
        struct hostent *HA;

        gethostname(hostName,200);
        HA = gethostbyname(hostName);
        strcpy(hostIP,inet_ntoa(*(struct in_addr*)HA->h_addr));
}
char *getVMIPaddressbyNode(char *vmip,int nodenum)
{
        char vmid[10];
        sprintf(vmid,"vm%d",nodenum);

        struct hostent *host = gethostbyname(vmid);
        inet_ntop(AF_INET,host->h_addr,vmip,INET_ADDRSTRLEN);
        if (host != NULL )
                return vmip;
        else
                return NULL;
}

int createDomaimDGramSock(char *tempPath)
{
        struct sockaddr_un localAddr;
        socklen_t localAddrLen;
        int sockfd;
	bzero(&localAddr,sizeof(localAddr));

        sockfd=Socket(AF_LOCAL,SOCK_DGRAM,0);
        if(sockfd<0)
        {
                perror("socket");
        }

        bzero(&localAddr,sizeof(localAddr));
        localAddr.sun_family=AF_LOCAL;
        printf("\n path:%s \n",tempPath);
        strcpy(localAddr.sun_path,tempPath);
        unlink(tempPath);
        localAddrLen=sizeof(localAddr);
        if((bind(sockfd,(struct sockaddr*)&localAddr,localAddrLen))<0)
        {
                perror("Bind");
        }

	return sockfd;
}

int getHostVMNo(){
    char hostName[1024];
    int hostNo;

    gethostname(hostName, 10);
    hostNo = atoi(hostName+2);
    if (hostNo < 1 || hostNo > 10) {
        err_msg("Warning: Invalid hostname '%s'", hostName);
    }
        printf(" \n node no inside function:%d \n",hostNo);
    return hostNo;
}
int getVMByIP(char *addr){
	struct in_addr sa;
	int vm_no = 0;
	struct hostent *host = NULL;

	int i = inet_pton(AF_INET,addr,&sa);
	
	if (i > 0) {
		host = gethostbyaddr(&sa,sizeof(sa),AF_INET);
		char *local = host->h_name;
		local++;
		local++;
		vm_no = atoi(local);
	}
	return vm_no;
}
/*
int sendwaitingpackets(routing_entry *routingTable,IfaceInfo *iFaceDataList,int dest,int map_hw_to_iface[])
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
*/
void setODRPacket(packet *packet, int type, char *srcIP, char *dstIP,
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


}

