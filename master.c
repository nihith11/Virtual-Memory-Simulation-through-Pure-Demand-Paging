#include <stdio.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

const int pagesize=2*1024;

typedef struct{
    int free;
    int pid;
}frame;

typedef struct{
    int *pagetable;
    int *v_inv;
    int n_p;
}pagetable;

int main(){
    int k,max_pages,frame_num;
    printf("Enter the number of processes:");
    scanf("%d",&k);
    printf("enter the maximum no of pages per process:");
    scanf("%d",&max_pages);
    printf("enter total number of frames in memory:");
    scanf("%d",&frame_num);
    printf("%d %d %d\n",k,max_pages,frame_num);

    int framelistid = shmget(ftok("./home",50),frame_num*sizeof(frame),IPC_CREAT|0666);
    /* if(framelistid==-1){
        perror("shmget");
        exit(1);
    } */
    frame *list = (frame*)shmat(framelistid,NULL,0);
    /* if((void*)list==(void*)-1){
        perror("shmat");
        exit(1);
    } */
    for(int i=0;i<frame_num;i++){
        list[i].free=1;
        list[i].pid=-1;
    }
    for(int i=0;i<frame_num;i++){
        printf("%d-%d   ",list[i].free,list[i].pid);
    }

    int mq1,mq2,mq3;

    int ptid = shmget(ftok("./home",51),k*sizeof(pagetable),IPC_CREAT|0666);
    if(ptid==-1){
        perror("shmget");
        exit(1);
    }

    srand(time(NULL));
    pagetable *pro_page = (pagetable*)shmat(ptid,NULL,SHM_RND);
    if((void*)pro_page==(void*)-1){
        perror("shmat");
        exit(1);
    }

    for(int i=0;i<k;i++){
        pro_page[i].n_p = rand()%max_pages + 1;
        printf("%d ",pro_page[i].n_p);
    }
    printf("\n");
    shmdt(list);
    shmdt(pro_page);
    shmctl(framelistid,IPC_RMID,0);
    shmctl(ptid,IPC_RMID,0);
}

/*             if((v=rand())%10<2)
                num=no+rand()%10; */

/*for(int i=0;i<k;i++){
         int no;
        no= 
                pro_page[i].n_p = rand()%max_pages + 1;
        printf("%d ",pro_page[i].n_p);
/*         pro_page[i].pagetable = (int *)malloc(no*sizeof(int));
        pro_page[i].v_inv = (int*)malloc(no*sizeof(int));
        for(int j=0;j<no;j++){
            pro_page[i].v_inv[j]=0;
        }
        int len=rand()%(8*no +1) + 2*no;
        ref[i]=(char**)malloc(len*sizeof(char*));
        for(int j=0;j<len;j++){
            ref[i][j]=(char*)malloc(5);
            int num,v;
            num = rand()%no + 1;

            sprintf(ref[i][j],"%d",num);
        } */
       /* printf("%d",no); */
    /*}

    /*     
    for(int i=0;i<k;i++){
        int no;
        no=pro_page[i].n_p = rand()%max_pages + 1;
        pro_page[i].pagetable = (int *)malloc(no*sizeof(int));
        pro_page[i].v_inv = (int*)malloc(no*sizeof(int));
        for(int j=0;j<no;j++){
            pro_page[i].v_inv[j]=0;
        }
        int len=rand()%(8*no +1) + 2*no;
        ref[i]=(char**)malloc(len*sizeof(char*));
        for(int j=0;j<len;j++){
            ref[i][j]=(char*)malloc(5);
            int num,v;
            num = rand()%no + 1;

            sprintf(ref[i][j],"%d",num);
        }
    }

    for(int i=0;i<k;i++){
        int tot=pro_page[i].n_p;
        for(int j=0;j<tot;j++){
            printf("%s ",ref[i][j]);
        }
        printf("\n");
    }
    */