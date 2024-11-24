#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>

typedef struct{
	long mtype;
	char mbuf;
} msgmmu;

typedef struct{
	long  mtype;
	int id;
} msg;

int send_message( int qid, msg *qbuf ){
	int result, length;
	length = sizeof( msg) - sizeof(long);
	if ((result = msgsnd( qid, qbuf, length, 0)) == -1){
		perror("Error in sending message");
		exit(1);
	}
	return (result);
}

int read_message( int qid, long type,msg *qbuf ){
	int result, length;
	length = sizeof(msg) - sizeof(long);
	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1){
		perror("Error in receiving message");
		exit(1);
	}
	return (result);
}

int read_mmu( int qid, long type,msgmmu *qbuf ){
	int result, length;
	length = sizeof(msgmmu) - sizeof(long);
	if ((result = msgrcv(qid, qbuf, length, type,  0)) == -1){
		perror("Error in receiving message");
		exit(1);
	}
	return (result);
}

int main(int argc , char * argv[]){
	int master_pid;
	int mq1 = atoi(argv[1]);
	int mq2 = atoi(argv[2]);
	int k = atoi(argv[3]);
	master_pid = atoi(argv[4]);
	int semid = atoi(argv[5]);

	msg msg_send, msg_recv;

	printf("Total No. of Process received = %d\n", k);
	int terminated_process = 0;

	while (1){
		read_message(mq1, 10, &msg_recv);
		int curr_id = msg_recv.id;

		msg_send.mtype = 20 + curr_id;
		send_message(mq1, &msg_send);

		msgmmu mmu_recv;
		read_mmu(mq2, 0, &mmu_recv);

		if (mmu_recv.mtype == 5){
			msg_send.mtype = 10;
			msg_send.id=curr_id;
			send_message(mq1, &msg_send);
		}
		else if (mmu_recv.mtype == 10){
			terminated_process++;
		}
		else{
			perror("Wrong message from mmu\n");
			exit(1);
		}
		if (terminated_process == k)
			break;
	}
	kill(master_pid, SIGUSR1);
	struct sembuf sop;
	sop.sem_num = 0;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	semop(semid, &sop, 1);
	printf("Scheduler terminating ...\n") ;
	exit(1) ;
}
