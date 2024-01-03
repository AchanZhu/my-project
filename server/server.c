#include "server.h"
//服务器
int flag=0;
pthread_mutex_t mylock;
void *pthread_fun(void* arg){
    pthread_detach(pthread_self());
    pthread_arg my_arg=*((pthread_arg *)arg);
    Msg_t myMsg={0};
    int size=sizeof(Msg_t)-sizeof(long);
    int i=0,sum=0;
    char buff[32]={0};
    char buf[128]={0},id[20]={0},tmp[20]={0};
    read(my_arg.fd,id,sizeof(id));
    sprintf(buf,"INSERT INTO connect VALUES(\"%s\",%d)",id,my_arg.fd);
    pthread_mutex_lock(&mylock);
	sqlite3_exec(my_arg.db,buf,NULL,NULL,NULL);
    pthread_mutex_unlock(&mylock);
    while(id[i]!='\0')
        sum+=id[i++];
    while(1){
        while(1){//从消息队列读数据，直到读到对应用户名的为止
            memset(&myMsg,0,sizeof(myMsg));
            msgrcv(my_arg.msqid,&myMsg,size,sum,0);
            if(0==strcmp(myMsg.userinfo,id)){
                break;
            }else{
                msgsnd(my_arg.msqid,&myMsg,size,0);
            }
            usleep(200000);
        }
        if(myMsg.recvtype==5){
            fcntl(my_arg.fd,F_SETFL,fcntl(my_arg.fd,F_GETFL)|O_NONBLOCK);//非阻塞
            if(0==read(my_arg.fd,&buff,sizeof(buff))){
                memset(buf,0,sizeof(buf));
                sprintf(buf,"DELETE FROM connect WHERE id=\"%s\"",id);//断开连接
                pthread_mutex_lock(&mylock);
                sqlite3_exec(my_arg.db,buf,NULL,NULL,NULL);
                pthread_mutex_unlock(&mylock);
                epoll_ctl(my_arg.epfd,EPOLL_CTL_DEL,my_arg.fd,NULL);
                close(my_arg.fd);
                pthread_exit(NULL);
            }else{
                fcntl(my_arg.fd,F_SETFL,fcntl(my_arg.fd,F_GETFL)&(~O_NONBLOCK));//阻塞
                continue;
            }
        }
        write(my_arg.fd,&myMsg,sizeof(Msg_t));
        memset(&myMsg,0,sizeof(myMsg));
        if(read(my_arg.fd,&myMsg,sizeof(Msg_t))==0){
            memset(buf,0,sizeof(buf));//断开连接
            sprintf(buf,"DELETE FROM connect WHERE id=\"%s\"",id);
            pthread_mutex_lock(&mylock);
            sqlite3_exec(my_arg.db,buf,NULL,NULL,NULL);
            pthread_mutex_unlock(&mylock);
            epoll_ctl(my_arg.epfd,EPOLL_CTL_DEL,my_arg.fd,NULL);
            close(my_arg.fd);
            pthread_exit(NULL);
        }
        if(myMsg.recvtype==4){
            myMsg.msgtype+=1;//sum+1
            msgsnd(my_arg.msqid,&myMsg,size,0);
        }
        if(myMsg.recvtype==1){
            myMsg.msgtype+=2;
            msgsnd(my_arg.msqid,&myMsg,size,0);
        }
    }
}
int main(int argc, const char *argv[]){
    pthread_mutex_init(&mylock,NULL);
    int lfd,cfd,i,fd,epfd,num,size;
    char buff[64]={0};
    pthread_t tid;
    sqlite3 *db;
    pthread_arg *arg=(pthread_arg *)calloc(1,sizeof(pthread_arg));
    key_t key=ftok("/home/linux",'a');
    int msqid=msgget(key,IPC_CREAT|0666);
    arg->msqid=msqid;
    system("rm ~/achan/my-project/boa/server/mytable.db");
    sqlite3_open("mytable.db",&db);//打开数据库2
    sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS connect(id TEXT PRIMARY KEY,fd INT)",NULL,NULL,NULL);
    lfd=socket(AF_INET,SOCK_STREAM,0);//创建监听套接字
    struct sockaddr_in server_msg_st={AF_INET,htons(8888),inet_addr("192.168.250.100")};//信息结构体
    struct sockaddr_in client_msg_st={0};
    bind(lfd,(const struct sockaddr *)&server_msg_st,sizeof(server_msg_st));//绑定
    listen(lfd,5);//设置监听
    struct timeval timval = {0};
    timval.tv_sec = 2;
    timval.tv_usec = 0;
    //设置超时：
    setsockopt(lfd,SOL_SOCKET,SO_RCVTIMEO,&timval,sizeof(timval));
    epfd=epoll_create(100);//创建epoll树
	struct epoll_event ep_event={0};
	ep_event.events=EPOLLIN;//监测读事件
	ep_event.data.fd=lfd;
	epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ep_event);//将监听套接字添加到epoll树上
	struct epoll_event *ep_events=calloc(N,sizeof(struct epoll_event));//纪录信息的结构体
    printf("server start!\n");
    int client_size=sizeof(client_msg_st);
    while(true){
		num=epoll_wait(epfd,ep_events,N,-1);
		for(i=0;i<num;i++){
			if(lfd==(fd=ep_events[i].data.fd)){//连接事件
				cfd=accept(lfd,(struct sockaddr *)&client_msg_st,&client_size);
                sprintf(arg->ip,"%s",inet_ntoa(client_msg_st.sin_addr));
                printf("ip=%s connect success\n",arg->ip);
				ep_event.events=EPOLLIN|EPOLLET;//读事件边沿触发
				ep_event.data.fd=cfd;
				epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ep_event);//添加连接的fd
                arg->fd=cfd;
                arg->epfd=epfd;
                arg->db=db;
                pthread_create(&tid,NULL,pthread_fun,(void*)arg);//创建线程
		    }
        }
	}
	close(lfd);
    free(ep_events);
	return 0;
}