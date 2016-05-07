#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>

#define MSGSIZE 5000

struct message_Client
{
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};

#define MSGSZ sizeof(struct message_Client)-sizeof(long)

int main()
{	
	int msgKEY,semKEY,sharedKEY,i,j,n,op,opi,f,amount,retVal,balance,tempID;
	pid_t pid;
	struct sembuf SOP;
	pid = getpid();

	FILE *atmLocator;
	char buff[1000],tempBuff[1000],atmID[1000];

	while(1)
	{
		printf("Choose an option:\n");
		printf("1. Enter an ATM\n");
		printf("2. Exit\n");

		scanf("%d",&opi);

		if(opi == 2)
		{
			break;
		}

		else if(opi == 1)
		{
			printf("ENTER ATM ID: ");
			scanf("%s",atmID);

			// locate ATM ID in ATM Locator

			atmLocator = fopen("ATM_Locator.txt","r");
			f = 0;

			while(fgets(buff,1000,atmLocator) != NULL)
			{
				i = 0;
				j=0;
				n = strlen(buff);
				while(buff[i]!='\t' && i<n)
				{
					tempBuff[j] = buff[i];
					i++;
					j++;
				}
				tempBuff[j] = '\0';
				i++;
				if(strcmp(tempBuff,atmID) == 0)
				{
					f = 1;
					j=0;
					while(buff[i]!='\t' && i<n)
					{
						tempBuff[j] = buff[i];
						i++;
						j++;
					}
					tempBuff[j] = '\0';
					msgKEY = atoi(tempBuff);
					i++;

					j = 0;

					while(buff[i]!='\t' && i<n)
					{
						tempBuff[j] = buff[i];
						i++;
						j++;
					}
					tempBuff[j] = '\0';
					semKEY = atoi(tempBuff);
					i++;

					j = 0;

					while(buff[i]!='\t' && i<n)
					{
						tempBuff[j] = buff[i];
						i++;
						j++;
					}
					tempBuff[j] = '\0';
					sharedKEY = atoi(tempBuff);
					break;
				}
			}

			fclose(atmLocator);

			if(f == 0)
			{
				printf("Sorry, ATM not Found...Try Again\n");
				continue;
			}

			int msgID = msgget(msgKEY,IPC_CREAT|0666);
	    	if(msgID < 0)printf("Message Get Error in msgKEY\n");

	    	int semid = semget(semKEY,1,IPC_CREAT|0666);
	    	if(semid == -1 )printf("Semaphore could not be created\n");

	    	printf("Waiting to Enter the ATM...\n");

	    	SOP.sem_num = 0;
	        SOP.sem_op = -1;
	        SOP.sem_flg = 0;
	        semop(semid, &SOP, 1);

	        printf("\033[H\033[J");

	        printf("ATMID: %s\tmsgKEY: %d\tsemKEY: %d\tsharedKEY: %d\n",atmID,msgKEY,semKEY,sharedKEY);

	        struct message_Client msgSend,msgRcv;

	        msgSend.mtype = 1;
	        msgSend.pid = pid;
	        strcpy(msgSend.mtext,"");
	        sprintf(msgSend.mtext,"1\t%d",pid);

	        if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) perror("msgsnd Error\n");
	        //else cout<<"Successfully Inserted in Q0: "<<msg.mtext<<endl;

	        if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
	        else printf("\n%s",msgRcv.mtext);

			while(1){
				printf("\nWelcome Client %d\n",pid);
				printf("Choose an option:\n");
				printf("1. Deposit Money\n");
				printf("2. Withdraw Money\n");
				printf("3. View Your Account Balance\n");
				printf("4. Leave ATM\n");
				scanf("%d",&op);

				if(op == 1)
				{
					// Deposit Money
					printf("Enter the Amount you want to deposit: ");
					scanf("%d",&amount);

					// Send Transaction Details TO atm.c
					strcpy(msgSend.mtext,"");
					sprintf(msgSend.mtext,"1\t%d",amount);
					if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

					if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
	        		else printf("\n%s",msgRcv.mtext);
				}

				else if(op == 2)
				{
					// Withdraw Money
					printf("Enter the Amount you want to withdraw: ");
					scanf("%d",&amount);

					// Request ATM.c
					strcpy(msgSend.mtext,"");
					sprintf(msgSend.mtext,"2\t%d",amount);
					if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

					if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
	        		else printf("\n%s",msgRcv.mtext);
				}

				else if(op == 3)
				{
					// View Balance

					// Request ATM.c
					strcpy(msgSend.mtext,"");
					sprintf(msgSend.mtext,"3\t0");
					if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

					if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
	        		//else printf("%s\n",msgRcv.mtext);

	        		sscanf(msgRcv.mtext,"2\t%d",&balance);

					printf("\nYour current Account Balance is: %d\n",balance);
				}

				else if(op == 4)
				{
					// Leave
					strcpy(msgSend.mtext,"");
					sprintf(msgSend.mtext,"4\t%d",pid);
					if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

					if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
	        		else printf("\n%s",msgRcv.mtext);

					// Release Lock

					SOP.sem_num = 0;
			        SOP.sem_op = 1;
			        SOP.sem_flg = 0;
			        semop(semid, &SOP, 1);
					break;
				}

				else{
					printf("Invalid option....Try Again\n");
				}
			}
		}
	}

	return 0;
}