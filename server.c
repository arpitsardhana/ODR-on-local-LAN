#include "utility.h"

int main()
{
	int sockfd, cliPort,servVMNo;
	struct sockaddr_un cliAddr,servAddr;
	time_t rawtime;
	socklen_t servAddrLen,cliAddrLen;
	char data[512],buf[1024],servIP[512],filePath[1024],cliIP[512];
        memset(filePath,'\0',1024);
        memset(servIP,'\0',1024);
        memset(cliIP,'\0',1024);

	
	
	getTempPath(filePath,SERV_TEMP_FILE,0);
	sockfd=createDomaimDGramSock(filePath);
	
	servVMNo=getHostVMNo();
	getVMsIP(servIP);
	printf("\nServer is running on %s in VM %d ",servIP,servVMNo);
	
	while(1)
	{
	memset(buf,'\0',1024);
	memset(data,'\0',512);
	printf("\nblocked in recv from");
	cliAddrLen=sizeof(cliAddr);
	//strcpy(cliIP,"127.0.0.1");
	
	//Recvfrom(sockfd, buf, sizeof(buf), 0,  (struct sockaddr*)&cliAddr,&cliAddrLen );
	msg_recv(sockfd, buf, cliIP, &cliPort);
	printf("Connection recved from client %s ",buf);
	
	time(&rawtime);
	memset(data,'\0',512);
	strcpy(data,ctime(&rawtime));
	
	printf("\ntime in server:%s",data);
	//Sendto(sockfd, data, sizeof(data), 0, (struct sockaddr*)&cliAddr, cliAddrLen);
	msg_send(sockfd, data, 1, cliIP, cliPort);
	}
	close(sockfd);
	
}
