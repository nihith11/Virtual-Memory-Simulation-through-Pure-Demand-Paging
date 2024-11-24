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
#include <netdb.h>
#include <signal.h>
#include <sys/shm.h>

int pg_no[1000] ;
int no_of_pages;

typedef struct{
	long mtype;
	int id;
	int p_num;
} mmu_send;

typedef struct{
	long    mtype;
	int f_num;
} mmumsg_recv;

typedef struct{
	long mtype;
	int id;
} msg;

void conv_ref_pg_no(char * refs){
	const char s[2] = "|";
	char *token;
	token = strtok(refs, s);
	while ( token != NULL )
	{
		pg_no[no_of_pages] = atoi(token);
		no_of_pages++;
		token = strtok(NULL, s);
	}
}

int send_mmu( int qid, mmu_send *qbuf ){
	int result, length;
	length = sizeof(mmu_send) - sizeof(long);
	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)	{
		perror("Error in sending message");
		exit(1);
	}
	return (result);
}
int recv_mmu( int qid, long type, mmumsg_recv *qbuf ){
	int result, length;
	length = sizeof(mmumsg_recv) - sizeof(long);
	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1){
		perror("Error in receiving message");
		exit(1);
	}
	return (result);
}

int send_msg( int qid, msg *qbuf ){
	int result, length;
	length = sizeof(msg) - sizeof(long);
	if ((result = msgsnd( qid, qbuf, length, 0)) == -1)	{
		perror("Error in sending message");
		exit(1);
	}
	return (result);
}
int rcv_msg( int qid, long type, msg *qbuf ){
	int result, length;
	length = sizeof( msg) - sizeof(long);
	if ((result = msgrcv( qid, qbuf, length, type,  0)) == -1){
		perror("Error in receiving message");
		exit(1);
	}
	return (result);
}

int main(int argc, char *argv[]) 
{
	int id;
	int mq1, mq3;
	id = atoi(argv[1]);
	mq1 = atoi(argv[2]);
	mq3  = atoi(argv[3]);
	no_of_pages = 0;
	conv_ref_pg_no(argv[4]);
	printf("Process id= %d\n", id);

	msg msg_send;
	msg_send.mtype = 10;
	msg_send.id = id;
	send_msg(mq1, &msg_send);

	msg msg_recv;
	rcv_msg(mq1, 20 + id, &msg_recv);

	mmu_send mmu_send;
	mmumsg_recv mmu_recv;
	int cpg = 0; //counter for page number array
	while (cpg < no_of_pages){
		printf("Sent request for %d page number\n", pg_no[cpg]);
		mmu_send.mtype = 10;
		mmu_send.id = id;
		mmu_send.p_num = pg_no[cpg];
		send_mmu(mq3, &mmu_send);

		recv_mmu(mq3, 20 + id, &mmu_recv);
		if (mmu_recv.f_num >= 0){
			printf("Frame number from MMU received for process %d: %d\n" , id, mmu_recv.f_num);
			cpg++;
		}
		else if (mmu_recv.f_num == -1){
			printf("Page fault occured for process %d\n", id);
		}
		else if (mmu_recv.f_num == -2){
			printf("Invalid page reference for process %d terminating ...\n", id) ;
			exit(1);
		}
	}
	printf("Process %d Terminated successfly\n", id);
	mmu_send.p_num = -9;
	mmu_send.id = id;
	mmu_send.mtype = 10;
	send_mmu(mq3, &mmu_send);

	exit(1);
	return 0;
}
