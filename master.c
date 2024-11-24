#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/sem.h>
#include <math.h>

typedef struct{
	int f_num;
	int v_inv;
	int count;
}ptbentry;

typedef struct{
	pid_t pid;
	int m;
	int f_cnt;
	int f_allo;
}pro_info;

typedef struct{
	int current;
	int flist[];
}freelist;

int flag = 0;
int pid,spid,mpid;
int ptbid, freelid;
int readyid, msgq2id, msgq3id;
int pcbid,semid;

void removeall(){
	shmctl(ptbid,IPC_RMID, NULL);
	shmctl(freelid,IPC_RMID, NULL);
	shmctl(pcbid,IPC_RMID, NULL);
	msgctl(readyid, IPC_RMID, NULL);
	msgctl(msgq2id, IPC_RMID, NULL);
	msgctl(msgq3id, IPC_RMID, NULL);
}

void quit(int status){
	removeall();
	exit(status);
}

int main(int argc, char const *argv[]){
	int k,m,f;
	srand(time(NULL));
	signal(SIGINT, quit);

	printf("Enter no of processes:");
	scanf("%d",&k);
	printf("Enter maximum no of pages in a process:");
	scanf("%d",&m);
	printf("Enter no of frames in physical address in memory:");
	scanf("%d",&f);

	pid = getpid();
	if(k <= 0 || m <= 0 || f <=0 || f < k){
		printf("Invalid input\n");
		exit(1);
	}

	ptbid = shmget(ftok(".",100), m*sizeof(ptbentry)*k, 0666 | IPC_CREAT | IPC_EXCL);
	ptbentry *ptb = (ptbentry*)(shmat(ptbid, NULL, 0));
	for(int i=0;i<k*m;i++){
		ptb[i].f_num = -1;
		ptb[i].v_inv = 0;
	}

	freelid = shmget(ftok(".",101), sizeof(freelist)+f*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL);
	freelist *allframes = (freelist*)(shmat(freelid, NULL, 0));
	for(int i=0;i<f;i++){
		allframes->flist[i] = i;
	}
	allframes->current = f-1;

	pcbid = shmget(ftok(".",102), sizeof(pro_info)*k, 0666 | IPC_CREAT | IPC_EXCL );
	pro_info *allinfo = (pro_info*)(shmat(pcbid, NULL, 0));

	int totpages = 0;
	for(int i=0;i<k;i++){
		allinfo[i].pid = i;
		allinfo[i].m = rand()%m + 1;
		allinfo[i].f_allo = 0;
		totpages +=  allinfo[i].m;
	}
	int allo_frame = 0;
	printf("tot = %d, k = %d, f=  %d\n",totpages,k,f);
	int max = 0,maxi = 0;
	for(int i=0;i<k;i++)
	{
		allinfo[i].pid = -1;
		int allo = (int)round(allinfo[i].m*(f-k)/(float)totpages) + 1;
		if(allinfo[i].m > max)
		{
			max = allinfo[i].m;
			maxi = i;
		}
		allo_frame = allo_frame + allo;
		allinfo[i].f_cnt = allo;
	}
	allinfo[maxi].f_cnt += f - allo_frame; 

	readyid = msgget(ftok(".",103), 0666 | IPC_CREAT| IPC_EXCL);
	msgq2id = msgget(ftok(".",104), 0666 | IPC_CREAT| IPC_EXCL );
	msgq3id = msgget(ftok(".",105), 0666 | IPC_CREAT| IPC_EXCL);

	semid = semget(ftok(".",106), 1, 0666 | IPC_CREAT);

	char st1[20],st2[20],st3[20],st4[20],st5[20],st6[20],st7[20],st8[20],st9[20],st10[20],st11[20],st12[20];
	sprintf(st1,"%d",readyid);
	sprintf(st2,"%d",msgq2id);
	sprintf(st3,"%d",msgq3id);
	sprintf(st4,"%d",ptbid);
	sprintf(st5,"%d",freelid);
	sprintf(st6,"%d",pcbid);
	sprintf(st7,"%d",m);
	sprintf(st8,"%d",k);
	sprintf(st9,"%d",f);
	sprintf(st10,"%d",pid);
	sprintf(st12,"%d",semid);

	if((spid = fork()) == 0){
		execlp("./scheduler","./scheduler",st1,st2,st8,st10,st12,(char *)(NULL));
	}

	if((mpid = fork()) == 0){
		execlp("xterm","xterm","-T","MMU","-e","./mmu",st2,st3,st4,st5,st6,st7,st8,(char *)(NULL) );
	}

	printf("generating processed\n");
    for(int i=0;i<k;i++){
		int rlen = rand()%(8*allinfo[i].m) + 2*allinfo[i].m + 1;
		char rstring[m*20*40];
		printf("rlen = %d\n",rlen);
		int l = 0;
		for(int j=0;j<rlen;j++)
		{
			int r;
			r = rand()%allinfo[i].m;
			float p = (rand()%100)/100.0;
			if(p < 0.2)
			{
				r = rand()%(1000*m) + allinfo[i].m;
			}
			l += sprintf(rstring+l,"%d|",r);
		}
		printf("Ref string = %s\n",rstring);
		if(fork() == 0){
			sprintf(st11,"%d",i);
			execlp("./process","./process",st11,st1,st3,rstring,(char *)(NULL));
		}
		usleep(250*1000);	
	}

    struct sembuf sop;
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	semop(semid, &sop, 1);
	
    shmdt(ptb);
    shmdt(allinfo);
    shmdt(allframes);
	removeall();
	return 0;
}
