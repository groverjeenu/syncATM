// Assignment 6
// ATM Simulator

// Objective
// In this assignment, you have to manage an ATM system, where clients can perform various operations like withdraw money, deposit money, or check balance.

// Group Details
// Group No: 22
// Member 1: Jeenu Grover (13CS30042)
// Member 2: Ashish Sharma (13CS30043)

// Filename: atm.c

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
#include <sys/time.h>

#define MAX 10000
#define MSGSIZE 5000

struct message_Client
{
    long mtype;
    pid_t pid;
    char mtext[MSGSIZE];
};

#define MSGSZ sizeof(struct message_Client)-sizeof(long)

typedef struct TransactionInfo{
	long long int clientID;
	long long int timeStamp;
	long long int balance;
	long long int transaction;
}TransactionInfo;

typedef struct ATMLocal{
	int counter;
	TransactionInfo TI[MAX];
} ATMLocal;


#define SIZE sizeof(ATMLocal)

ATMLocal * atmMem;
long long int time_in_mill;

int myID;


int getLastbalance(int clientID)
{
    int c = atmMem->counter;

    int i;


    for(i=c-1;i>=0;i--)
    {
        if(atmMem->TI[i].clientID == clientID)
            return atmMem->TI[i].balance;
    }

    return 0;

}


void printLocal()
{
    int c = atmMem->counter;
    int i;

    printf("\n---------------------------------ATM %d-------------------------------------\n",myID);
    printf("ClientID\tTimeStamp\t\tPrevBalance\tTransaction\n");

    printf("---------------------------------------------------------------------------\n");

    for(i=c-1;i>=0;i--)
    {
        printf("%lld\t\t%lld\t\t%lld\t\t%lld\n",atmMem->TI[i].clientID,atmMem->TI[i].timeStamp,atmMem->TI[i].balance,atmMem->TI[i].transaction);
    }

    printf("---------------------------------------------------------------------------\n\n");
}




int getLocalAmounts(int clientID)
{
    char buffer[1000];
    FILE * ptr = fopen("ATM_Locator.txt","r");
    int atmid,atm_cli_key,semKey,shmKey,shmid,ctr,i,j,change = 0,base;
    TransactionInfo *TI;

    ATMLocal * atmptr = NULL;
    while(fgets(buffer,1000,ptr) != NULL)
    {
        sscanf(buffer,"%d\t%d\t%d\t%d\n",&atmid,&atm_cli_key,&semKey,&shmKey);

        shmid = shmget(shmKey,SIZE,IPC_CREAT|0666);
        atmptr =  (ATMLocal*)shmat(shmid,NULL,0);

        ctr = atmptr->counter;
        TI = atmptr->TI;

        //printf("ATMid : %d ctr : %d\n",atmid,ctr);

        

        for(i=0;i<ctr;i++)
        {
            if(TI[i].clientID == clientID)
            {
                change += TI[i].transaction;
            }
        }

        shmdt((char*)atmptr);



    }


    return (change + getLastbalance(clientID));
}

int main(int argc, char* argv[])
{

	myID = atoi(argv[1]);
	int master_atm_key = atoi(argv[2]);
	int atm_client_key = atoi(argv[3]);
	int semKEY = atoi(argv[4]);
	int sharedMemKey = atoi(argv[5]);

	int atm_client = msgget(atm_client_key,IPC_CREAT|0666);
    if(atm_client < 0)printf("Message Get Error in ATM->CLIENT\n");

    int master_atm= msgget(master_atm_key,IPC_CREAT|0666);
    if(master_atm < 0)printf("Message Get Error in MASTER->ATM\n");

    int shmid = shmget(sharedMemKey,SIZE,IPC_CREAT|0666);
    char * shmPtr =  (char*)shmat(shmid,NULL,0);

    int semid = semget(semKEY,1,IPC_CREAT|0666);
    if(semid == -1 )printf("Semaphore could not be created\n");
    if(semctl(semid,0,SETVAL,1) == -1)printf("Value of 1st subsemphore could not be set\n");

    atmMem = (ATMLocal*)shmPtr;

    struct message_Client clientMsg,masterMsg;

    int clientID = -1;

    int mypid =getpid();
    char temp[1000];
    int x,y,ex,localAmt;
    atmMem->counter = 0;
    struct timeval tp;

    printf("ATMID: %d\tmsgKEY: %d\tsemKEY: %d\tsharedKEY: %d\n",myID,atm_client_key,semKEY,sharedMemKey);


    while(1)
    {
    	if(msgrcv(atm_client ,&clientMsg,MSGSZ,0,0) < 0 ) printf("Msgrcv Error\n");
        printf("Client %d Entered\n",clientMsg.pid);

        clientID = clientMsg.pid;

        masterMsg.mtype = mypid;
        masterMsg.pid = mypid;
        sprintf(temp,"1\t%d\n",clientID);
        strcpy(masterMsg.mtext,temp);

        if(msgsnd(master_atm,&masterMsg,MSGSZ,0)<0)printf("Sending Failure to master\n");

        if(msgrcv(master_atm ,&masterMsg,MSGSZ,mypid,0) < 0 ) printf("Msgrcv Error\n");
        else printf("%s\n",masterMsg.mtext);
        sscanf(masterMsg.mtext,"%d\t%d",&x,&y);

        clientMsg.mtype = mypid;
        clientMsg.pid = mypid;
        if(y == 0)
        {
        	strcpy(clientMsg.mtext,"Client already Registered\n");
            printf("Client already Registered\n");
        }
        else if(y == 1)
        {
        	strcpy(clientMsg.mtext,"New a/c created\n");
            printf("New a/c Created\n");
        }

        //printf("counter: %lld\n",atmMem->counter);

        printLocal();

        if(msgsnd(atm_client,&clientMsg,MSGSZ,0)<0)printf("Sending Failure to client\n");

        ex = 0;

        while(1)
        {
        	if(msgrcv(atm_client ,&clientMsg,MSGSZ,0,0) < 0 ) printf("Msgrcv Error\n");
            //else printf("%s\n",clientMsg.mtext);
        	sscanf(clientMsg.mtext,"%d\t%d",&x,&y);

        	clientMsg.mtype = mypid;
        	clientMsg.pid = mypid;



        	if(x == 1)
        	{
                // Deposit Request
                printf("Client %d: DEPOSIT %d to My Account\n",clientID,y);
        		if(y>=0)
        		{
        			atmMem->TI[atmMem->counter].clientID = clientID;
        			gettimeofday(&tp,NULL);
                    time_in_mill = (tp.tv_sec) * 1000 + (tp.tv_usec) / 1000 ;
        			atmMem->TI[atmMem->counter].timeStamp = time_in_mill;
        			atmMem->TI[atmMem->counter].balance = getLastbalance(clientID);
                    atmMem->TI[atmMem->counter].transaction = y;
                    atmMem->counter++;
                    sprintf(temp,"Successfully Deposited Rs. %d to your account.\n",y);
                    strcpy(clientMsg.mtext,temp);

        		}
                else
                {
                    strcpy(clientMsg.mtext,"Please Enter Valid Amount\n");
                }
        	}
        	else if(x == 2)
        	{
        		// Withdraw Request

                printf("Client %d: Withdraw %d from My Account\n",clientID,y);

                if(y >= 0)
                {
                    printf("Running a Local Consistency Check for a/c %d\n",clientID);
                    localAmt = getLocalAmounts(clientID);
                    if(localAmt >= y)
                    {
                        atmMem->TI[atmMem->counter].clientID = clientID;
                        gettimeofday(&tp,NULL);
                        time_in_mill = (tp.tv_sec) * 1000 + (tp.tv_usec) / 1000 ;
                        atmMem->TI[atmMem->counter].timeStamp = time_in_mill;
                        atmMem->TI[atmMem->counter].balance = getLastbalance(clientID);
                        atmMem->TI[atmMem->counter].transaction = -y;
                        atmMem->counter++;
                        sprintf(temp,"Successfully withdrawn Rs. %d from your account.\n",y);
                        strcpy(clientMsg.mtext,temp);
                    }
                    else
                    {
                        strcpy(clientMsg.mtext,"Sorry, You do not have sufficient balance in your account.\n");
                    }
                }
                else
                {
                    strcpy(clientMsg.mtext,"Please Enter Valid Amount\n");
                }
        	}
        	else if(x == 3)
        	{
        		//view
                printf("Client %d: View Balance\n",clientID);

                masterMsg.mtype = mypid;
                masterMsg.pid = mypid;
                sprintf(temp,"3\t%d",clientID);
                strcpy(masterMsg.mtext,temp);

                if(msgsnd(master_atm,&masterMsg,MSGSZ,0)<0)printf("Sending View Balance message to master FAILED\n");

                if(msgrcv(master_atm ,&masterMsg,MSGSZ,mypid,0) < 0 ) printf("Msgrcv Error\n");

                strcpy(clientMsg.mtext,masterMsg.mtext);
        	}
        	else if(x == 4)
        	{
        		sprintf(clientMsg.mtext,"Good Bye Client %d\n",clientID);
        		ex = 1;
        	}
        	else
        	{
        		strcpy(clientMsg.mtext,"Invalid Request\n");
        	}

            printLocal();

        	if(msgsnd(atm_client,&clientMsg,MSGSZ,0)<0)printf("Sending Failure to client\n");
        	if(ex == 1)
            {
                printf("Client %d Exited Successfully\n",clientID);
                break;
            }
        }

    }

	return 0;
}