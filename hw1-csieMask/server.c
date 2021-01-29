#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* connect_greeting = "Please enter the id (to check how many masks can be ordered):\n";
const char* write_order = "Please enter the mask type (adult or children) and number of mask you would like to order:\n";
const char* locked = "Locked.\n";
const char* op_failed = "Operation failed.\n";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id; //customer id
    int adultMask;
    int childrenMask;
} Order;

int handle_read(request* reqP) {
	char buf[512];
    int nread=read(reqP->conn_fd, buf, sizeof(buf));
    if(nread<=0)
	    return -1;
    else{
    	memcpy(reqP->buf, buf, strlen(buf));
	return nread;
    }
}
void process_id(request* rq){
	int id=atoi(rq->buf);
	if(id>=902001 && id<=902020)
		rq->id=id;
	else
		strcpy(rq->buf,op_failed);
}
int preorder(request* rq,int *num,int *type,Order *rqorder){
	char age[512];
	int n;
	sscanf(rq->buf,"%s %d",age,&n);
	*num=n;
	char adult[]="adult";
	char children[]="children";
	if(strcmp(age,adult)==0){
		*type=1;
		if((rqorder->adultMask)<(*num) || (*num)<=0)
			return 0;
		else{
			(rqorder->adultMask)-=(*num);
			return 1;
		}
	}
	else if(strcmp(age,children)==0){
		*type=2;
		if((rqorder->childrenMask)<(*num) || (*num)<=0)
			return 0;
		else{
			(rqorder->childrenMask)-=(*num);
			return 1;
		}
	}
	else
		return 0;
}
int read_lock(int fd,off_t offset,int whence,off_t len){
	struct flock lock;
	int cmd=F_SETLK;
	lock.l_type=F_RDLCK;
	lock.l_start=offset;
	lock.l_whence=whence;
	lock.l_len=len;
	return(fcntl(fd,cmd,&lock));
}
int write_lock(int fd,off_t offset,int whence,off_t len){
	struct flock lock;
	int cmd=F_SETLK;
	lock.l_type=F_WRLCK;
	lock.l_start=offset;
	lock.l_whence=whence;
	lock.l_len=len;
	return(fcntl(fd,cmd,&lock));
}
int un_lock(int fd,off_t offset,int whence,off_t len){
	struct flock lock;
	int cmd=F_SETLK;
	lock.l_type=F_UNLCK;
	lock.l_start=offset;
	lock.l_whence=whence;
	lock.l_len=len;
	return(fcntl(fd,cmd,&lock));
}
void close_fd(request *rq,fd_set *allrset){
	close(rq->conn_fd);
	FD_CLR(rq->conn_fd,allrset);
        free_request(rq);
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    fd_set allrset,crset;
    FD_ZERO(&allrset);
    FD_ZERO(&crset);
    FD_SET(svr.listen_fd,&allrset);
    fprintf(stderr,"listen_fd %d\n",svr.listen_fd);

#ifdef READ_SERVER
	int recfd=open("preorderRecord",O_RDONLY);
	if(recfd<0)
		fprintf(stderr,"open error\n");
#else
	int recfd=open("preorderRecord",O_RDWR);
	if(recfd<0)
		fprintf(stderr,"open error\n");
#endif
	int wlock[20]={0};
	Order order[20];
	for(int i=0;i<20;i++){
		order[i].id=0;
	}

    while (1) {
        // TODO: Add IO multiplexing
	    memcpy(&crset,&allrset,sizeof(allrset));
	    int ready=select(maxfd,&crset,NULL,NULL,NULL);
	    if(ready<=0)
		    continue;
	    if(FD_ISSET(svr.listen_fd,&crset)){
        	// Check new connection
		    fprintf(stderr,"accept new connection\n");
        	clilen = sizeof(cliaddr);
        	conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
        	if (conn_fd < 0) {
            		if (errno == EINTR || errno == EAGAIN) continue;  // try again
            		if (errno == ENFILE) {
                		(void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                		continue;
            		}
            		ERR_EXIT("accept");
        	}
        	requestP[conn_fd].conn_fd = conn_fd;
        	strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
        	fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);	
        	int nwrite=write(conn_fd,connect_greeting,strlen(connect_greeting));
		    if(nwrite<=0){
			    fprintf(stderr,"write once connected failed\n");
			    free_request(&requestP[conn_fd]);
			    close(conn_fd);
			    continue;
		    }
		    FD_SET(requestP[conn_fd].conn_fd,&allrset);
	    }
    
        // TODO: handle requests from clients
        for(int i=svr.listen_fd+1;i<maxfd;i++){
            if(FD_ISSET(i,&crset)){
                int ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
                if(ret<=0){
                    if(requestP[i].wait_for_write==1){
                        wlock[requestP[i].id-902001]--;
                        if(wlock[requestP[i].id-902001]==0){
                            int pos=(requestP[i].id-902001)*sizeof(Order);
                            un_lock(recfd,pos,SEEK_SET,sizeof(Order));
                        }
                    }
                    write(requestP[i].conn_fd,op_failed,strlen(op_failed));
                    close_fd(&requestP[i],&allrset);
                    continue;
                }
                if(requestP[i].wait_for_write==0){
                    process_id(&requestP[i]);
                    if(requestP[i].id==0){
                        write(requestP[i].conn_fd,op_failed,strlen(op_failed));
                        close_fd(&requestP[i],&allrset);
                        continue;
                    }
                }
#ifdef READ_SERVER     
				//check same id in same process
				int pos=(requestP[i].id-902001)*sizeof(Order);
				fprintf(stderr,"%d\n",pos);
				int lk=read_lock(recfd,pos,SEEK_SET,sizeof(Order));
				if(lk<0){
					if(errno==EAGAIN || errno==EACCES){
						write(requestP[i].conn_fd,locked,strlen(locked));
						close_fd(&requestP[i],&allrset);
						continue;
					}
					fprintf(stderr,"lk error\n");
				}
				Order sorder;
				int nread;
				if(lseek(recfd,pos,SEEK_SET)>=0){
					nread=read(recfd,&sorder,sizeof(Order));
					fprintf(stderr,"%d\n",nread);
					if(nread<0)
						fprintf(stderr,"read error\n");
				}
				else
					fprintf(stderr,"lseek error\n");
				sprintf(requestP[i].buf,"You can order %d adult mask(s) and %d children mask(s).\n",sorder.adultMask,sorder.childrenMask);
				write(requestP[i].conn_fd,requestP[i].buf,strlen(requestP[i].buf));
				close_fd(&requestP[i],&allrset);
				un_lock(recfd,pos,SEEK_SET,sizeof(Order));
#else
        		//check same id in same process
			    if(requestP[i].wait_for_write==0){
                    if(wlock[requestP[i].id-902001]>0){
                        write(requestP[i].conn_fd,locked,strlen(locked));
                        close_fd(&requestP[i],&allrset);
                        continue;
                    }
                    int pos=(requestP[i].id-902001)*sizeof(Order);
                    int lk=write_lock(recfd,pos,SEEK_SET,sizeof(Order));
                    if(lk<0){
                        if(errno==EAGAIN || errno==EACCES){
                            write(requestP[i].conn_fd,locked,strlen(locked));
                            close_fd(&requestP[i],&allrset);
                            continue;
                        }
                    }
                    wlock[requestP[i].id-902001]++;
                    int nread;
                    Order tmp;
                    if(lseek(recfd,pos,SEEK_SET)>=0){
                        nread=read(recfd,&tmp,sizeof(Order));
                        fprintf(stderr,"%d\n",nread);
                        if(nread<0)
                            fprintf(stderr,"read error\n");
                    }
                    else
                        fprintf(stderr,"lseek error\n");
                    order[requestP[i].id-902001]=tmp;
                    sprintf(requestP[i].buf,"You can order %d adult mask(s) and %d children mask(s).\n",tmp.adultMask,tmp.childrenMask);
                    write(requestP[i].conn_fd,requestP[i].buf,strlen(requestP[i].buf));
                    sprintf(requestP[i].buf,write_order,strlen(write_order));
                    write(requestP[i].conn_fd,requestP[i].buf,strlen(requestP[i].buf));
                    requestP[i].wait_for_write=1;
                    continue;
                }
                //TODO preorder
                if(requestP[i].wait_for_write==1){
                    int num,type;
                    int correct=preorder(&requestP[i],&num,&type,&order[requestP[i].id-902001]);
                    if(correct==0){
                        wlock[requestP[i].id-902001]--;
                        if(wlock[requestP[i].id-902001]==0){
                            int pos=(requestP[i].id-902001)*sizeof(Order);
                            un_lock(recfd,pos,SEEK_SET,sizeof(Order));
                        }
                        write(requestP[i].conn_fd,op_failed,strlen(op_failed));
                        close_fd(&requestP[i],&allrset);
                        continue;
                    }
                    int pos=(requestP[i].id-902001)*sizeof(Order);
                    int nwrite=0;
                    if(lseek(recfd,pos,SEEK_SET)>=0){
                        nwrite=write(recfd,&order[requestP[i].id-902001],sizeof(Order));
                        if(nwrite<0)
                            fprintf(stderr,"write error\n");
                        if(type==1){
                            sprintf(requestP[i].buf,"Pre-order for %d successed, %d adult mask(s) ordered.\n",requestP[i].id,num);
                            write(requestP[i].conn_fd,requestP[i].buf,strlen(requestP[i].buf));
                        }
                        else if(type==2){
                            sprintf(requestP[i].buf,"Pre-order for %d successed, %d children mask(s) ordered.\n",requestP[i].id,num);
                            write(requestP[i].conn_fd,requestP[i].buf,strlen(requestP[i].buf));
                        }
                    }
                    else
                        fprintf(stderr,"lseek error\n");
                    wlock[requestP[i].id-902001]--;
                    if(wlock[requestP[i].id-902001]==0)
                        un_lock(recfd,pos,SEEK_SET,sizeof(Order));
                    close_fd(&requestP[i],&allrset);
                }
#endif
		    }//if ISSET
        }//for read set
    }
    close(recfd);
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write=0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}