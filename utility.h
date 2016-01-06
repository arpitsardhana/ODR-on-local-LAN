#ifndef _UTILITY_H
#define _UTILITY_H

#include "unp.h"
#include "odr.h"
#define TOTAL_NO_VMS   10
#define SERV_TEMP_FILE    "/arpsingh_servFile"
#define CLI_TEMP_FILE    "/arpssingh_clientFile_XXXXXX"
#define ODR_TEMP_FILE    "/arpsing_odrFile"
#define SERVER_PORT      21 
#define CLIENT_TIMEOUT 10
#define PAYLOAD 512

typedef struct pck_Data{
        char canonicalIP[512];
        int port;
        char data[PAYLOAD];
        int flag;
}pckData;
void getTempPath(char *filePath,char *fileName,int flag);
void msg_send(int sockfd, char *data, int flag,char *destIP, int destPort);
void msg_recv(int sockfd, char *data, char *sourceIP, int *sourcePort);
int createDomaimDGramSock(char *tempPath);
int getClientVMinfo();

#endif

