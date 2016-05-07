// Assignment 6
// ATM Simulator

// Objective
// In this assignment, you have to manage an ATM system, where clients can perform various operations like withdraw money, deposit money, or check balance.

// Group Details
// Group No: 22
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: master.c

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
#include <sys/shm.h>

#define MasterType 1000

#define NUM_CLIENTS 100000
#define MAX 10000
#define MSGSIZE 5000

typedef struct BalanceInfo{
	long long int clientID;
	long long int timeStamp;
	long long int balance;
}BalanceInfo;


typedef struct ATMGlobal
{
	int counter;
	BalanceInfo BI[NUM_CLIENTS];
}ATMGlobal;

typedef struct TransactionInfo{
	long long int clientID;
	long long int timeStamp;
	long long int balance;
	long long int transaction;
}TransactionInfo;

typedef struct ATMLocal{
	int counter;
	TransactionInfo TI[MAX];
}ATMLocal;

struct message_Client
{
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};

struct message_ATM
{
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};


#define MSGSZ sizeof(struct message_ATM)-sizeof(long)


ATMGlobal *ATMs;
ATMLocal *ATMLocals;
long long int time_in_mill;

void printGlobal()
{
    int c = ATMs->counter;
    int i;

    printf("\n----------------Global Memory--------------------------\n");
    printf("ClientID\tTimeStamp\t\tBalance\n");

    printf("-------------------------------------------------------\n");

    for(i=c-1;i>=0;i--)
    {
        printf("%lld\t\t%lld\t\t%lld\n",ATMs->BI[i].clientID,ATMs->BI[i].timeStamp,ATMs->BI[i].balance);
    }

    printf("-------------------------------------------------------\n\n");
}


void main(int argc,char *argv[])
{
	// n msgKEY(Master->ATM) InitialSemKey InitialSharedMemoryKey
	if(argc<5)
	{
		printf("USAGE: %s Num_ATMs msgKEY SemKEY SharedMemKEY\n",argv[0]);
        exit(1);
	}
	int msgKEY,semKEY,shmKEY,n,i;
	char ATMid[1000],msgKEY_ATM[1000],msgKEY_Client[1000],semKEY_ATM[1000],shmKEY_ATM[1000];
	int *ATM,*msgKEY_client,*semKEY_atm,*shmKEY_atm,*shmKEY_atm_ID;
	struct timeval tv;

	int *pid_ATMs;
	FILE *fptr;

	n = atoi(argv[1]);
	msgKEY = atoi(argv[2]);
	semKEY = atoi(argv[3]);
	shmKEY = atoi(argv[4]);

	pid_ATMs = (int *)(malloc((n+1)*sizeof(int)));
	ATM = (int *)(malloc((n+1)*sizeof(int)));
	msgKEY_client= (int *)(malloc((n+1)*sizeof(int)));
	semKEY_atm = (int *)(malloc((n+1)*sizeof(int)));
	shmKEY_atm = (int *)(malloc((n+1)*sizeof(int)));
	shmKEY_atm_ID = (int *)(malloc((n+1)*sizeof(int)));

	int msgID = msgget(msgKEY,IPC_CREAT|0666);
	if(msgID < 0) printf("Message Get Error in msgKEY\n");

	int shmID = shmget((key_t)shmKEY,sizeof(ATMGlobal),IPC_CREAT|0666);

	close(open("ATM_Locator.txt", O_RDWR|O_CREAT, 0666));
    fptr = fopen("ATM_Locator.txt","w");

	for(i=1;i<=n;++i)
	{
		ATM[i] = i;
		msgKEY_client[i] = msgKEY+i;
		semKEY_atm[i] = semKEY+i;
		shmKEY_atm[i] = shmKEY+i;

		strcpy(ATMid,"");
		strcpy(msgKEY_ATM,"");
		strcpy(msgKEY_Client,"");
		strcpy(semKEY_ATM,"");
		strcpy(shmKEY_ATM,"");

		sprintf(ATMid,"%d",ATM[i]);
		sprintf(msgKEY_ATM,"%d",msgKEY);
		sprintf(msgKEY_Client,"%d",msgKEY_client[i]);
		sprintf(semKEY_ATM,"%d",semKEY_atm[i]);
		sprintf(shmKEY_ATM,"%d",shmKEY_atm[i]);

		pid_ATMs[i] = fork();
		if(pid_ATMs[i] == 0){
			// myID	msgKEY(Master->ATM) msgKEY(ATM->Client) SemKEY SharedMemKey  
			execlp("xterm","xterm" ,"-hold","-e","./atm",ATMid,msgKEY_ATM,msgKEY_Client,semKEY_ATM,shmKEY_ATM,(const char *)NULL);
			exit(1);
		}

		fprintf(fptr,"%d\t%d\t%d\t%d\n",ATM[i],msgKEY_client[i],semKEY_atm[i],shmKEY_atm[i]);
	}

	fclose(fptr);

	// Attach ATMGlobal
	ATMs = (ATMGlobal *)shmat(shmID,NULL,0);

	ATMs->counter = 0;

	// Get the shared Memory keys of other ATMs for global update

	for(i=1;i<=n;++i)
	{
		shmKEY_atm_ID[i] = shmget((key_t)shmKEY_atm[i],sizeof(ATMLocal),IPC_CREAT|0666);
	}

	while(1)
	{
		long long int i,j,k,ns,reqType,clientID,num,f,ATM_pid;
		char req[1000];
		struct message_ATM msgSend,msgRcv;

		if(msgrcv(msgID,&msgRcv,MSGSZ,0,0) < 0) printf("msgrcv Error\n");
        else printf("%s\n",msgRcv.mtext);
        ATM_pid = msgRcv.pid;
        //printf("PID: %d\n",ATM_pid);
        i = 0;
        j = 0;
        ns = strlen(msgRcv.mtext);

        while(msgRcv.mtext[i] != '\t' && i<ns)
        {
        	req[j] = msgRcv.mtext[i];
        	i++;
        	j++;
        }
        req[j] = '\0';
        reqType = atol(req);
        i++;

        if(reqType == 1)
        {
            printf("Verifying Client %lld\n",clientID);
        	j = 0;
        	while(i<ns)
        	{
        		req[j] = msgRcv.mtext[i];
        		j++;
        		i++;
        	}
        	req[j] = '\0';

        	clientID = atol(req);

        	// Search for Client in the Global Shared Memory

        	f = 0;
        	num = ATMs->counter;
        	for(k=0; k<num ; ++k)
        	{
        		if(((ATMs->BI)[k]).clientID == clientID)
        		{
        			f = 1;
        			break;
        		}
        	}

        	if(f == 1)
        	{
        		// Client Exists

                printf("Client already Registered\n");
        		strcpy(msgSend.mtext,"");
        		strcpy(msgSend.mtext,"1\t0");
        		msgSend.mtype = ATM_pid;
        		if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");
        	}

        	else if(f == 0)
        	{
        		// Client Does not Exists -- Create a new Entry
        		gettimeofday(&tv,NULL);
                time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        		((ATMs->BI)[ATMs->counter]).clientID = clientID;
        		((ATMs->BI)[ATMs->counter]).timeStamp = time_in_mill;
        		((ATMs->BI)[ATMs->counter]).balance = 0;
        		(ATMs->counter)++;

        		for(i=1;i<=n;++i)
        		{
        			ATMLocals = (ATMLocal *)shmat(shmKEY_atm_ID[i],NULL,0);
        			gettimeofday(&tv,NULL);
                    time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        			((ATMLocals->TI)[ATMLocals->counter]).clientID = clientID;
        			((ATMLocals->TI)[ATMLocals->counter]).timeStamp = time_in_mill;
        			((ATMLocals->TI)[ATMLocals->counter]).balance = 0;
        			((ATMLocals->TI)[ATMLocals->counter]).transaction = 0;
        			(ATMLocals->counter)++;

        			shmdt((char *)ATMLocals);
        		}
                printf("New a/c Created\n");
        		strcpy(msgSend.mtext,"");
        		strcpy(msgSend.mtext,"1\t1");
        		msgSend.mtype = ATM_pid;
        		if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

                printGlobal();
        	}

        	//printf("Sent Successfully\n");

        }

        else if(reqType == 3)
        {
            printf("Running a Global Consistency Check for a/c %lld\n",clientID);
        	int counter_ATM;
        	long long int totTransaction;
        	j = 0;
        	ns = strlen(msgRcv.mtext);
        	while(i<ns)
        	{
        		req[j] = msgRcv.mtext[i];
        		j++;
        		i++;
        	}
        	req[j] = '\0';

        	clientID = atol(req);

        	totTransaction = 0;
        	for(i=1;i<=n;++i)
        	{
        		ATMLocals = (ATMLocal *)shmat(shmKEY_atm_ID[i],NULL,0);
        		counter_ATM = (ATMLocals)->counter;

        		for(j=0;j<counter_ATM;++j)
        		{
        			if(((ATMLocals->TI)[j]).clientID == clientID)
        			{
        				totTransaction += ((ATMLocals->TI)[j]).transaction;
        				//printf("ATM %d\t%d\n",i,((ATMLocals->TI)[j]).transaction);
        			}
        		}
        		shmdt((char *)ATMLocals);
        	}

        	num = ATMs->counter;
        	for(k=0; k<num ; ++k)
        	{
        		if(((ATMs->BI)[k]).clientID == clientID)
        		{
        			gettimeofday(&tv,NULL);
                    time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        			totTransaction += ((ATMs->BI)[k]).balance;
        			((ATMs->BI)[k]).balance = totTransaction;
        			((ATMs->BI)[k]).timeStamp = time_in_mill;
        			break;
        		}
        	}

        	for(i=1;i<=n;++i)
        	{
        		ATMLocals = (ATMLocal *)(shmat(shmKEY_atm_ID[i],NULL,0));
        		counter_ATM = (ATMLocals)->counter;

        		for(j=0;j<counter_ATM;++j)
        		{
        			if(((ATMLocals->TI)[j]).clientID == clientID)
        			{
        				for(k=j;k<(counter_ATM)-1;++k)
        				{
        					((ATMLocals->TI)[k]).clientID = ((ATMLocals->TI)[k+1]).clientID;
        					((ATMLocals->TI)[k]).timeStamp = ((ATMLocals->TI)[k+1]).timeStamp;
        					((ATMLocals->TI)[k]).balance = ((ATMLocals->TI)[k+1]).balance;
        					((ATMLocals->TI)[k]).transaction = ((ATMLocals->TI)[k+1]).transaction;
        				}
        				counter_ATM--;
        				((ATMLocals)->counter)--;
        				j--;
        			}
        		}
        		gettimeofday(&tv,NULL);
                time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
        		((ATMLocals->TI)[ATMLocals->counter]).clientID = clientID;
    			((ATMLocals->TI)[ATMLocals->counter]).timeStamp = time_in_mill;
    			((ATMLocals->TI)[ATMLocals->counter]).balance = totTransaction;
    			((ATMLocals->TI)[ATMLocals->counter]).transaction = 0;
    			(ATMLocals->counter)++;

        		shmdt((char *)ATMLocals);
        	}
            printf("Done\n");
        	// Reply back to ATM
        	strcpy(msgSend.mtext,"");
    		sprintf(msgSend.mtext,"2\t%lld",totTransaction);
    		msgSend.mtype = ATM_pid;
    		if(msgsnd(msgID,&msgSend,MSGSZ,0) < 0) printf("msgsnd Error\n");

            printGlobal();
        }
	}
}