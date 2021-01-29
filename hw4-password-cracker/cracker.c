#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
//compile: gcc cracker.c -lpthread -lcrypto -o cracker
//execute: ./cracker ${prefix} ${goal} ${n-length} ${m-valid} ${outfile-not_abs}

//m thread return result store in data structure
//main responsible for join all threads, and find m length-1 str, then call thread to find the remain

typedef struct Thread_Data{
    int curr_i;
    char str_set[10][512];
}thread_data;

int n;  //n-treasure
char goal[512];

int treasure_hunt(thread_data* tid_data,char* prefix,int cnt,char* goal){
    int curr=tid_data->curr_i;
    //printf("curr,cnt: %d %d\n",curr,cnt);
    //printf("str: %s\n",prefix);
    if(curr>=n)
        return 1;
    if(cnt>4)
        return 0;
    
    int prelen=strlen(prefix);
    for(int i=33;i<127;i++){
        char add[5]={'\0'};
        add[0]=i;
        char pre[512]={'\0'};
        //int prelen=strlen(tid_data->str_set[curr]);
        strncpy(pre,prefix,prelen);
        pre[prelen]='\0';
        strncat(pre,add,1);

        //MD5
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX md5_buf;
        MD5_Init(&md5_buf);
        MD5_Update(&md5_buf,pre,strlen(pre));
        MD5_Final(hash,&md5_buf);
        
        char result[512]={'\0'};
        for(int j=0;j<MD5_DIGEST_LENGTH;j++){
            char buf[5]={'\0'};
	    sprintf(buf,"%02x",hash[j]);
	    //printf("%02x ",hash[j]);
            strcat(result,buf);
        }//printf("\n");
	    int cmpj=0;
        for(cmpj=0;cmpj<curr+1;cmpj++){
            if(result[cmpj]!=goal[cmpj]){
                //printf("not equal\n");
		        break;
	        }
        }
        if(cmpj==curr+1){
            strncpy(tid_data->str_set[curr+1],pre,strlen(pre));
            tid_data->str_set[curr+1][strlen(pre)]='\0';
            tid_data->curr_i++;
            if(treasure_hunt(tid_data,pre,0,goal)==1)
                return 1;
        }
    }
    if(cnt==4){
	    //printf("cnt??\n");
        return 0;
    }
    
    for (int i=33;i<127;i++){
        char add[5]={'\0'};
        add[0]=i;
	    char pre[512]={'\0'};
	    strcpy(pre,prefix);
	    pre[strlen(prefix)]='\0';
        strncat(pre,add,1);
        if(treasure_hunt(tid_data,pre,cnt+1,goal)==1)
            return 1;
    }
    return 0;
}
void *start_routine(void* tid_data){
    thread_data *tmp=(thread_data *)tid_data;
	int ret=treasure_hunt(tmp,tmp->str_set[0],0,goal);
    //printf("prepare exit\n");
    pthread_exit(NULL);
}
int main(int argc,char *argv[]){

    /*manage input and open file*/
    if(argc!=6)
        fprintf(stderr,"args error\n");
    char prefix[512]={'\0'};
    strcpy(prefix,argv[1]);
    //fprintf(stderr,"prefix: %s\n",prefix);

    strcpy(goal,argv[2]);
    //fprintf(stderr,"goal: %s\n",goal);
    n=atoi(argv[3]);
    //fprintf(stderr,"n: %d\n",n);
    int m=atoi(argv[4]);
    //fprintf(stderr,"m: %d\n",m);
    
    thread_data treasure[m];

    /*first m*/
    pthread_t tid[m];
    for(int i=33;i<33+m;i++){
        char add[5]={'\0'};
        add[0]=i;
        int prelen=strlen(prefix);
        strncpy(treasure[i-33].str_set[0],prefix,prelen);
        treasure[i-33].str_set[0][prelen]='\0';
        strncat(treasure[i-33].str_set[0],add,1);
        treasure[i-33].curr_i=0;
        int err=pthread_create(&tid[i-33],NULL,start_routine,(void *)&treasure[i-33]);
	if(err!=0)
        fprintf(stderr,"pthread_create error\n");
	    //printf("thread create\n");
    }
    for(int i=0;i<m;i++){
    	//printf("join thread\n");
        pthread_join(tid[i],NULL);
    }

    /*process file*/
    FILE *fp;
    fp=fopen(argv[5],"w+");
    if(fp==NULL)
        fprintf(stderr,"open file error\n");
    for(int i=0;i<m;i++){
        for(int j=1;j<=n;j++)
            fprintf(fp,"%s\n",treasure[i].str_set[j]);
        fprintf(fp,"===\n");
    }
    fclose(fp);
    return 0;
}
