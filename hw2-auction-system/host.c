#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
typedef struct Player_Data{
    int id;
    int wins;
    int rank;
}p_data;
void init_player(p_data *player){
    for(int i=0;i<8;i++){
        player[i].id=0;
        player[i].wins=0;
        player[i].rank=0;
    }
}
void find_add(int winner,p_data *player){
	int i;
    for(i=0;i<8;i++){
        if(player[i].id==winner)
            break;
    }
    player[i].wins++;
}
int compw(const void *a,const void *b){
    p_data c1=*(p_data *)a;
    p_data c2=*(p_data *)b;
    if(c1.wins>c2.wins)
        return -1;
    if(c1.wins==c2.wins)
        return 0;
    if(c1.wins<c2.wins)
        return 1;
}
int compid(const void *a,const void *b){
    p_data c1=*(p_data *)a;
    p_data c2=*(p_data *)b;
    if(c1.id<c2.id)
        return -1;
    if(c1.id==c2.id)
        return 0;
    if(c1.id<c2.id)
        return 1;
}
void ranking(p_data *player){
    qsort(player,8,sizeof(p_data),compw);
    int j=0;
    int r=1;
    for(int i=0;i<8;i+=j){
        j=0;
        while(i+j<8 && player[i].wins==player[i+j].wins){
            player[i+j].rank=r;
            j++;
        }r+=j;
    }
    qsort(player,8,sizeof(p_data),compid);
    /*fprintf(stderr,"--------------------\n");
    for(int i=0;i<8;i++){
	fprintf(stderr,"%d %d\n",player[i].id,player[i].rank);
    }fprintf(stderr,"--------------------\n");*/
}
int main(int argc,char *argv[]){
    /*printf("%d ",argc);
    printf("%s %s %s\n",argv[1],argv[2],argv[3]);*/
    int host_id=atoi(argv[1]);
    int key=atoi(argv[2]);
    int depth=atoi(argv[3]); //start from 0
    //printf("%d %d %d\n",host_id,key,depth);
    
    char infifo[512]; 
    //prepare read, write fifo
    FILE *fp_in,*fp_out;
    if(depth==0){
        sprintf(infifo,"fifo_%d.tmp",host_id);
        //fprintf(stderr,"%s\n",infifo);
        fp_in=fopen(infifo,"r");
        if(fp_in==NULL)
            fprintf(stderr,"open fifo %d error\n",host_id);
        
        fp_out=fopen("fifo_0.tmp","w");
        if(fp_out==NULL)
            fprintf(stderr,"open fifo 0 error\n");
    }
    
    int fd00[2],fd01[2],fd10[2],fd11[2];
    if(depth<2 && depth>=0){    //fork child hosts
        
        //fd*0 read from child, 1 write
        int err00,err01,err10,err11=0;
        
        err00=pipe(fd00);   err01=pipe(fd01);
        if(err00==-1 || err01==-1){
            perror("pipe"); exit(EXIT_FAILURE);
        }

        pid_t cpid=fork();
        if(cpid==-1){
            perror("fork"); exit(EXIT_FAILURE);
        }
        if(cpid==0){    //first child
                close(fd00[0]); close(fd01[1]);
                dup2(fd01[0],STDIN_FILENO);    close(fd01[0]);
                dup2(fd00[1],STDOUT_FILENO);    close(fd00[1]);
                char newd[10];
		sprintf(newd,"%d",depth+1);
                char *exeargv[]={"host",argv[1],argv[2],newd,NULL};
                int ret=execv("./host",exeargv);
                if(ret<0){
                    perror("err on execv");
                }
        }else{
            
            err10=pipe(fd10);   err11=pipe(fd11);
            if(err10==-1 || err11==-1){
                perror("pipe"); exit(EXIT_FAILURE);
            }
            
            cpid=fork();
            if(cpid==-1){
                perror("fork"); exit(EXIT_FAILURE);
            }
            if(cpid==0){    //second child
                close(fd10[0]); close(fd11[1]);
                dup2(fd11[0],STDIN_FILENO);    close(fd11[0]);
                dup2(fd10[1],STDOUT_FILENO);    close(fd10[1]);
                char newd[10];
		sprintf(newd,"%d",depth+1);
                char *exeargv[]={"host",argv[1],argv[2],newd,NULL};
                int ret=execv("./host",exeargv);
                if(ret<0){
                    perror("err on execv");
                }
            }
            else{   //parent
                close(fd00[1]); close(fd01[0]); close(fd10[1]); close(fd11[0]);
            }
        }
    }
    
    //loop until ending message
    FILE *pipe00;   FILE *pipe01;   FILE *pipe10;   FILE* pipe11;
    if(depth==0){
        //read fifo
        pipe01=fdopen(fd01[1],"w"); pipe11=fdopen(fd11[1],"w");
        pipe00=fdopen(fd00[0],"r"); pipe10=fdopen(fd10[0],"r");
        while(true){
            p_data player[8];
            init_player(player);

            fscanf(fp_in,"%d %d %d %d %d %d %d %d",&(player[0].id),&(player[1].id),&(player[2].id),&(player[3].id),&(player[4].id),&(player[5].id),&(player[6].id),&(player[7].id));
            fprintf(pipe01,"%d %d %d %d\n",player[0].id,player[1].id,player[2].id,player[3].id);
            fflush(pipe01);
            fprintf(pipe11,"%d %d %d %d\n",player[4].id,player[5].id,player[6].id,player[7].id);
            fflush(pipe11);
            if(player[0].id==-1)
                break;  //TODO:break?
            
            for(int round=0;round<10;round++){
                int win1,win2,bid1,bid2;
                fscanf(pipe00,"%d %d",&win1,&bid1);
                fscanf(pipe10,"%d %d",&win2,&bid2);
                if(bid1>bid2)
                    find_add(win1,player);
                else
                    find_add(win2,player);
            }
            ranking(player);
            fprintf(fp_out,"%d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n",key,player[0].id,player[0].rank,player[1].id,player[1].rank,player[2].id,player[2].rank,player[3].id,player[3].rank,player[4].id,player[4].rank,player[5].id,player[5].rank,player[6].id,player[6].rank,player[7].id,player[7].rank);
            fflush(fp_out);
        }
        int status;
        wait(&status);  wait(&status);
        close(fd00[0]);    close(fd10[0]);  close(fd01[1]); close(fd11[1]);
        fclose(fp_out); fclose(fp_in);
    }
    if(depth==1){
        int player[5];
        pipe01=fdopen(fd01[1],"w"); pipe11=fdopen(fd11[1],"w");
        pipe00=fdopen(fd00[0],"r"); pipe10=fdopen(fd10[0],"r");
        while(true){
            scanf("%d %d %d %d",&player[1],&player[2],&player[3],&player[4]);
            fprintf(pipe01,"%d %d\n",player[1],player[2]);
            fflush(pipe01);
            fprintf(pipe11,"%d %d\n",player[3],player[4]);
            fflush(pipe11);
            if(player[1]==-1)
                break;  //TODO:break?

            //10 rounds
            for(int round=0;round<10;round++){
                int win1,win2,bid1,bid2;
                fscanf(pipe00,"%d %d",&win1,&bid1);
                fscanf(pipe10,"%d %d",&win2,&bid2);
                if(bid1>bid2){
                    printf("%d %d\n",win1,bid1);
                    fflush(stdout);
                }
                else{
                    printf("%d %d\n",win2,bid2);
                    fflush(stdout);
                }
            }
        }
        int status;
        wait(&status);  wait(&status);
        close(fd00[0]);    close(fd10[0]);  close(fd01[1]); close(fd11[1]);
    }
    if(depth==2){
        while(true){
            int player[3];
            scanf("%d %d",&player[1],&player[2]);
            
            if(player[1]==-1)
                break;    //TODO:break?

            int pfd00[2],pfd10[2];
            pipe(pfd00);
            pid_t cpid=fork();
            if(cpid==0){    //first child
                close(pfd00[0]);
                dup2(pfd00[1],STDOUT_FILENO);
                char newid[10];
                sprintf(newid,"%d",player[1]);
                char *exeargv[]={"player",newid,NULL};
                int ret=execv("./player",exeargv);
                if(ret<0){
                    perror("err on execv");
                }
            }
            else{
                pipe(pfd10);
                cpid=fork();
                if(cpid==0){    //second child
                    close(pfd10[0]);
                    dup2(pfd10[1],STDOUT_FILENO);
                    char newid[10];
                    sprintf(newid,"%d",player[2]);
                    char *exeargv[]={"player",newid,NULL};
                    int ret=execv("./player",exeargv);
                    if(ret<0){
                        perror("err on execv");
                    }
                }
                else{   //parent:leaf host
                    close(pfd00[1]); close(pfd10[1]);
                    FILE *pfp00=fdopen(pfd00[0],"r");   FILE *pfp10=fdopen(pfd10[0],"r");
                    
                    //10 rounds
                    for(int round=0;round<10;round++){
                        int win1,win2,bid1,bid2;
                        fscanf(pfp00,"%d %d",&win1,&bid1);
                        fscanf(pfp10,"%d %d",&win2,&bid2);
                        if(bid1>bid2){
                            printf("%d %d\n",win1,bid1);
                            fflush(stdout);
                        }
                        else{
                            printf("%d %d\n",win2,bid2);
                            fflush(stdout);
                        }
                    }
                    
                    //wait player exit
                    int status;
                    wait(&status);  wait(&status);
                    close(pfd00[0]);    close(pfd10[0]);
                }
            }
        }
    }

    return 0;
}
