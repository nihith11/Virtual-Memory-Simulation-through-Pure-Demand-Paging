#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <limits.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/wait.h>

int count = 0;
int *pffreq;
FILE *output;

typedef struct{
	pid_t pid;
	int m;
	int f_cnt;
	int f_allo;
} pro_info;

struct msg{
	long mtype;
	int id;
	int pageno;
};

typedef struct{
	int current;
	int flist[];
} freelist;

struct msgpro{
	long mtype;
	int frameno;
};

struct msgsch{
	long mtype;
	char mbuf[1];
};

typedef struct{
	int frameno;
	int isvalid;
	int count;
} ptbentry;

int ptbid, freelid;
int mid2, mid3;
int pcbid;
int m,k;
int flag = 1;
pro_info *pcbptr;
ptbentry *ptbptr;
freelist *freeptr;

void closeFileAndExit(int signal) {
    if (output != NULL) {
        fclose(output);
    }
    exit(1);
}

int read_pro(int* id){
	struct msg mbuf;
	int length;
	length = sizeof(struct msg) - sizeof(long);
	memset(&mbuf, 0, sizeof(mbuf));
	int rst = msgrcv(mid3, &mbuf, length, 10, 0);
	*id = mbuf.id;
	return mbuf.pageno;
}

void send_pro(int id, int frameno){
	struct msgpro mbuf;
	mbuf.mtype = id + 20;
	mbuf.frameno = frameno;
	int length = sizeof(struct msg) - sizeof(long);
	int rst = msgsnd(mid3, &mbuf, length, 0);
}

void send_sched(int type){
	struct msgsch mbuf;
	mbuf.mtype = type;
	int length = sizeof(struct msg) - sizeof(long);
	int rst = msgsnd(mid2, &mbuf, length, 0);
}

int PFH(int id, int pageno){
	if (freeptr->current == -1 || pcbptr[id].f_cnt <= pcbptr[id].f_allo){
		int min = INT_MAX, mini = -1;
		int victim = 0;
		for (int i = 0; i < pcbptr[i].m; i++){
			if (ptbptr[id * m + i].isvalid == 1){
				if (ptbptr[id * m + i].count < min){
					min = ptbptr[id * m + i].count;
					victim = ptbptr[id * m + i].frameno;
					mini = i;
				}
			}
		}
		ptbptr[id * m + mini].isvalid = 0;
		return victim;
	}
	else{
		int fn = freeptr->flist[freeptr->current];
		freeptr->current -= 1;
		return fn;
	}
}

void freepages(int i){
	for (int k = 0; k < pcbptr[i].m; i++){
		if (ptbptr[i * m + k].isvalid == 1){
			freeptr->flist[freeptr->current + 1] = ptbptr[i * m + k].frameno;
			freeptr->current += 1;
		}
	}
}

int main(int argc, char const *argv[]){
	int terminated_process=0;
	mid2 = atoi(argv[1]);
	mid3 = atoi(argv[2]);
	ptbid = atoi(argv[3]);
	freelid = atoi(argv[4]);
	pcbid = atoi(argv[5]);
	m = atoi(argv[6]);
	k = atoi(argv[7]);
	pffreq = (int *)malloc(k*sizeof(int));
	for(int i=0;i<k;i++){
		pffreq[i] = 0;
	} 
	output = fopen("result.txt","w");
	pcbptr = (pro_info*)(shmat(pcbid, NULL, 0));
	ptbptr = (ptbentry*)(shmat(ptbid, NULL, 0));
	freeptr = (freelist*)(shmat(freelid, NULL, 0));
	while(flag){
		int id = -1, pageno;
		pageno = read_pro(&id);
		if(pageno == -1 && id == -1){
			continue;
		}
		int i = id;
		if (pageno == -9){
			freepages(id);
			send_sched(10);
			terminated_process++;
			continue;
		}
		count ++;
		printf("Page reference : (%d,%d,%d)\n",count,id,pageno);
		fprintf(output,"Page reference : (%d,%d,%d)\n",count,id,pageno);
		if (pcbptr[id].m < pageno || pageno < 0){
			printf("Invalid Page Reference : (%d %d)\n",id,pageno);
			fprintf(output,"Invalid Page Reference : (%d %d)\n",id,pageno);
			send_pro(id, -2);
			printf("Process %d: TRYING TO ACCESS INVALID PAGE REFERENCE %d\n", id, pageno);
			fprintf(output,"Process %d: TRYING TO ACCESS INVALID PAGE REFERENCE %d\n", id, pageno);
			terminated_process++;
			freepages(id);
			send_sched(10);
		}
		else{
			if (ptbptr[i * m + pageno].isvalid == 0){
				printf("Page Fault : (%d, %d)\n",id,pageno);
				fprintf(output,"Page Fault : (%d, %d)\n",id,pageno);
				pffreq[id] += 1;
				send_pro(id, -1);
				int fno = PFH(id, pageno);
				ptbptr[i * m + pageno].isvalid = 1;
				ptbptr[i * m + pageno].count = count;
				ptbptr[i * m + pageno].frameno = fno;
				
				send_sched(5);
			}
			else{
				send_pro(id, ptbptr[i * m + pageno].frameno);
				ptbptr[i * m + pageno].count = count;
			}
		}
		if(terminated_process == k){
			shmdt(pcbptr);
			shmdt(ptbptr);
			shmdt(freeptr);
			break;
		}
		sleep(1);
	}	
	printf("Page fault Count for each Process:\n");	
	fprintf(output,"Page fault Count for each Process:\n");
	printf("Process Id\tFreq\n");
	fprintf(output,"Process Id\tFreq\n");
	for(int i = 0;i<k;i++)
	{
		printf("%d\t%d\n",i,pffreq[i]);
		fprintf(output,"%d\t%d\n",i,pffreq[i]);
	}
	fflush(output);
	fclose(output);
	return 0;
}
