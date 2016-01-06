#include "unp.h"
#include "utility.h"
#include <setjmp.h>

static sigjmp_buf retransmit;

static void sig_handler(int sigNum)
{
	printf("\nClient time out regenerating Request...\n");
	siglongjmp(retransmit, 1);
}
int main()
{
        int sockfd;
        struct sockaddr_un cliAddr,servAddr;
        char data[512],filePath[1024],servIP[1024],cliIP[1024], cliVM;
        memset(data,'\0',512);
        memset(filePath,'\0',1024);
        memset(servIP,'\0',1024);
        memset(cliIP,'\0',1024);

        socklen_t servAddrLen,cliAddrLen;

        getTempPath(filePath,CLI_TEMP_FILE,1);
        sockfd=createDomaimDGramSock(filePath);

        //getTempPath(filePath,CLI_TEMP_FILE);
        cliVM=getHostVMNo();
        getVMsIP(cliIP);
        printf("\nClient is running on %s in VM %d ",cliIP,cliVM);

        while(1)
        {
                int n, servPort,i;
		int forceDiscovery=0;
		int  timeOut,count=0;
                char servIP[1024], inputVM[10];
				char *Vmno;
				signal(SIGALRM, sig_handler);
				//alarm(1);
                while(1){
                        printf("Choose a server VM among 1 to 10:");
                        scanf("%s",inputVM);
						
						if(strcmp(inputVM,"quit")==0)
						{
							printf("Program Terminating!");
							exit(0);
						}
						if(inputVM[0]>= '1' && inputVM[0]<= '9')
						{
							n=atoi(inputVM);
							if(n>=1 && n<=10)
							break;
							else continue;
						}
						else {
						for(i=0;i<3;i++)
						inputVM[i]=tolower(inputVM[i]);
							
						if(inputVM[0]=='v')
						{
							if(inputVM[1]=='m')
							{
							Vmno=inputVM+2;
							n=atoi(Vmno);
							if(n>=1 && n<=10)
								break;
							}
							else
								continue;
						}
						else 
							continue;
						}
                
						}	
                getVMsIP(servIP);
				getVMIPaddressbyNode(servIP,n);
                servPort=SERVER_PORT;
                //strcpy(servIP,"127.0.0.1");
                printf("\nSYN sent to server");
	retransmit:
		memset(data,'\0',512);
                strcpy(data,"Send me the time");
                msg_send(sockfd, data,forceDiscovery,servIP, servPort);
                printf("\n client at node vm %d sending request to server at vm %d \n",cliVM,n);
				alarm(CLIENT_TIMEOUT);
				
				if (sigsetjmp(retransmit, 1) != 0) 
				{
					if(count==1)
					{
						printf("\n Server Not reachable!!! \n");
						alarm(0);
						goto client;
						//exit(0);
					}
					forceDiscovery=1;
					count++;
					goto retransmit;
				}
		memset(data,'\0',512);	
                msg_recv(sockfd, data, servIP, &servPort);
                printf("\n Data from server:%s \n", data);
	client:
		alarm(0);
				
				
				
        }

        unlink(cliAddr.sun_path);
        remove(cliAddr.sun_path);
        close(sockfd);
		

}

