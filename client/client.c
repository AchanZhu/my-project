#include <my_head.h>
#include "client.h"
//客户机
pthread_mutex_t mylock;
Limitset_t mylimit={0};//阈值
void led_on_all(int fd){
    led_on(fd, LED1);
    led_on(fd, LED2);
    led_on(fd, LED3);
    led_on(fd, LED4);
    led_on(fd, LED5);
    led_on(fd, LED6);
}
void led_off_all(int fd){
    led_off(fd, LED1);
    led_off(fd, LED2);
    led_off(fd, LED3);
    led_off(fd, LED4);
    led_off(fd, LED5);
    led_off(fd, LED6);
}
void led_on(int fd, int which){
    ioctl(fd, LED_ON, &which);
}
void led_off(int fd, int which){
    ioctl(fd, LED_OFF, &which);
}
void *pthread_interrupt(void* arg){//按键控制数码管显示环境数据
    char a;
    char buff[20];
    int sun,temp,hum;
    pthread_arg *my_arg=(pthread_arg *)arg;
    while(1){
        read(my_arg->fd6,&a,1);
        sprintf(buff,"%f",my_arg->tmpMsg->env.sunvalue);
        write(my_arg->fd7,buff,5);

        read(my_arg->fd6,&a,1);
        sprintf(buff,"%f",my_arg->tmpMsg->env.tempvalue*10);
        write(my_arg->fd7,buff,4);

        read(my_arg->fd6,&a,1);
        sprintf(buff,"%f",my_arg->tmpMsg->env.humvalue*10);
        write(my_arg->fd7,buff,4);
    }
}
void *pthread_fan(void* arg){//控制风扇转动
    int fd5=*((int *)arg);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    int which=0;
    while(1){
        ioctl(fd5, FAN_ON, &which);
        usleep(300000);
    }
}
void *pthread_fun(void* arg){//环境数据监测
    pthread_detach(pthread_self());
    pthread_arg *my_arg=(pthread_arg *)arg;
    int which=0;
    int sun,temp,hum;
    while(1){
        ioctl(my_arg->fd1, GET_SUN, &sun);
        ioctl(my_arg->fd2, GET_TMP, &temp);
        ioctl(my_arg->fd2, GET_HUM, &hum);
        my_arg->tmpMsg->env.sunvalue = 0.35 * sun;//lx 370
        my_arg->tmpMsg->env.tempvalue = 175.72 * temp / 65536 - 46.85;//℃ 26
        my_arg->tmpMsg->env.humvalue = 125.0 * hum / 65536.0 - 6;//% 29
        if(mylimit.iullup<my_arg->tmpMsg->env.sunvalue){//光照
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.ledstate=0;
            led_off_all(my_arg->fd3);
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.iulldown>my_arg->tmpMsg->env.sunvalue){
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.ledstate=1;
            led_on_all(my_arg->fd3);
            pthread_mutex_unlock(&mylock);
        }

        if(mylimit.tempup<my_arg->tmpMsg->env.tempvalue){//温度
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.fanstate=0;
            if(*(my_arg->tidfan)>0){
                pthread_cancel(*(my_arg->tidfan));
                pthread_join(*(my_arg->tidfan),NULL);
                *(my_arg->tidfan)=0;
                usleep(200000);
                ioctl(my_arg->fd5, FAN_OFF, &which);
            }
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.tempdown>my_arg->tmpMsg->env.tempvalue){
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.fanstate=1;
            if(*(my_arg->tidfan)<=0){
                pthread_create(my_arg->tidfan,NULL,pthread_fan,(void*)&my_arg->fd5);
            }
            pthread_mutex_unlock(&mylock);
        }
        
        if(mylimit.humup<my_arg->tmpMsg->env.humvalue){//湿度
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.humstate=0;
            ioctl(my_arg->fd4, BEEP_OFF, &which);
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.humdown>my_arg->tmpMsg->env.humvalue){
            pthread_mutex_lock(&mylock);
            my_arg->tmpMsg->env.humstate=1;
            ioctl(my_arg->fd4, BEEP_ON, &which);
            pthread_mutex_unlock(&mylock);
        }
        usleep(700000);
    }
}
void *pthread_limit_set(void* arg){//阈值设置
    pthread_detach(pthread_self());
    Msg_t my_arg=*((Msg_t *)arg);
    char buff[20]={0};
    FILE *fp=fopen("limit.txt","w+");
    if(my_arg.limit.iullup>-0.1 && my_arg.limit.iullup<10000.1)
        mylimit.iullup=my_arg.limit.iullup;
    if(my_arg.limit.iulldown>-0.1 && my_arg.limit.iulldown<mylimit.iullup)
        mylimit.iulldown=my_arg.limit.iulldown;
    if(my_arg.limit.tempup>-0.1 && my_arg.limit.tempup<100.1)
        mylimit.tempup=my_arg.limit.tempup;
    if(my_arg.limit.tempdown>-0.1 && my_arg.limit.tempdown<mylimit.tempup)
        mylimit.tempdown=my_arg.limit.tempdown;
    if(my_arg.limit.humup>-0.1 && my_arg.limit.humup<100.1)
        mylimit.humup=my_arg.limit.humup;
    if(my_arg.limit.humdown>-0.1 && my_arg.limit.humdown<mylimit.humup)
        mylimit.humdown=my_arg.limit.humdown;
    sprintf(buff,"%.1f\n",mylimit.iullup);
    fputs(buff,fp);
    sprintf(buff,"%.1f\n",mylimit.iulldown);
    fputs(buff,fp);
    sprintf(buff,"%.1f\n",mylimit.tempup);
    fputs(buff,fp);
    sprintf(buff,"%.1f\n",mylimit.tempdown);
    fputs(buff,fp);
    sprintf(buff,"%.1f\n",mylimit.humup);
    fputs(buff,fp);
    sprintf(buff,"%.1f\n",mylimit.humdown);
    fputs(buff,fp);
    fclose(fp);
}
int main(int argc, const char *argv[]){
    int fd1,fd2,fd3,fd4,fd5,fd6,fd7;
    int sun,tmp,hum;
    float lux,rtmp,rhum;
    int which=0;
    char ip_addr[32]={0};
    char buff[64]={0};
    char id[20]={0};
    char password[20]={0};
    int port_num,fd=0;
    pthread_t tid=0,tid1=0,tidfan=0,limittid=0;
    pthread_arg my_arg={0};
    Msg_t myMsg={0};
    Msg_t tmpMsg={0};
    Msg_t limitMsg={0};
    char lim[6][20]={0};
    FILE *fp=fopen("limit.txt","r+");
    for(int i=0;i<6;i++){
        fgets(lim[i],12,fp);
        lim[i][strlen(lim[i])-1]='\0';
    }
    fclose(fp);
    mylimit.iullup=atof(lim[0]);
    mylimit.iulldown=atof(lim[1]);
    mylimit.tempup=atof(lim[2]);
    mylimit.tempdown=atof(lim[3]);
    mylimit.humup=atof(lim[4]);
    mylimit.humdown=atof(lim[5]);
    pthread_mutex_init(&mylock,NULL);
    int size=sizeof(Msg_t)-sizeof(long long);
    if((fd1 = open("/dev/ap3216", O_RDWR)) == -1)
        perror("open error1");
    if((fd2 = open("/dev/si7006", O_RDWR)) == -1)
        perror("open error2");
    if((fd3 = open("/dev/myleds", O_RDWR)) == -1)
        perror("open error3");
    if((fd4 = open("/dev/beep", O_RDWR)) == -1)
        perror("open error4");
    if((fd5 = open("/dev/fan", O_RDWR)) == -1)
        perror("open error5");
    if((fd6 = open("/dev/interrupt", O_RDWR)) == -1)
        perror("open error6");
    if((fd7 = open("/dev/m74hc595", O_RDWR)) == -1)
        perror("open error7");
    my_arg.fd1=fd1;
    my_arg.fd2=fd2;
    my_arg.fd3=fd3;
    my_arg.fd4=fd4;
    my_arg.fd5=fd5;
    my_arg.fd6=fd6;
    my_arg.fd7=fd7;
    my_arg.tmpMsg=&tmpMsg;
    my_arg.tidfan=&tidfan;
    pthread_create(&tid,NULL,pthread_fun,(void*)&my_arg);
    pthread_create(&tid1,NULL,pthread_interrupt,(void*)&my_arg);
    sprintf(ip_addr,"192.168.100.210");//scanf("%s",ip_addr);
    port_num=8888;//scanf("%d",&port_num);
    //创建套接字
    if((fd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket error");
        return -1;
    }
    //信息结构体
    struct sockaddr_in server_msg_st={AF_INET,htons(port_num),inet_addr(ip_addr)};
    //链接目标主机：
    sprintf(id,"abc");
    if(connect(fd,(const struct sockaddr*)&server_msg_st,sizeof(server_msg_st))==-1){
        perror("connect error");
        return -1;
    }
    printf("connect success\n");
    write(fd,id,sizeof(id));
    while(1){
        memset(&myMsg,0,sizeof(myMsg));
        read(fd,&myMsg,sizeof(myMsg));
        switch(myMsg.recvtype){
            case 1://设备控制
                if(myMsg.dev.led==1){
                    if(tmpMsg.env.sunvalue>mylimit.iullup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.ledstate=1;
                    led_on_all(fd3);
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(tmpMsg.env.sunvalue<mylimit.iulldown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.ledstate=0;
                    led_off_all(fd3);
                    pthread_mutex_unlock(&mylock);
                }
                if(myMsg.dev.fan==1){
                    if(tmpMsg.env.tempvalue>mylimit.tempup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.fanstate=1;
                    if(*(my_arg.tidfan)<=0){
                        pthread_create(my_arg.tidfan,NULL,pthread_fan,(void*)&fd5);
                    }
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(tmpMsg.env.tempvalue<mylimit.tempdown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.fanstate=0;
                    if(*(my_arg.tidfan)>0){
                        pthread_cancel(*(my_arg.tidfan));
                        pthread_join(*(my_arg.tidfan),NULL);
                        *(my_arg.tidfan)=0;
                        usleep(200000);
                        ioctl(fd5, FAN_OFF, &which);
                    }
                    pthread_mutex_unlock(&mylock);
                }
                if(myMsg.dev.beep==1){
                    if(tmpMsg.env.humvalue>mylimit.humup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.humstate=1;
                    ioctl(fd4, BEEP_ON, &which);
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(tmpMsg.env.humvalue<mylimit.humdown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.humstate=0;
                    ioctl(fd4, BEEP_OFF, &which);
                    pthread_mutex_unlock(&mylock);
                }
                break;
            case 3://阈值设置
                memcpy(&limitMsg,&myMsg,sizeof(limitMsg));
                pthread_create(&limittid,NULL,pthread_limit_set,(void*)&limitMsg);
                break;
        }
        myMsg.env.ledstate=tmpMsg.env.ledstate;
        myMsg.env.fanstate=tmpMsg.env.fanstate;
        myMsg.env.humstate=tmpMsg.env.humstate;
        myMsg.env.sunvalue=tmpMsg.env.sunvalue;
        myMsg.env.tempvalue=tmpMsg.env.tempvalue;
        myMsg.env.humvalue=tmpMsg.env.humvalue;
        write(fd,&myMsg,sizeof(Msg_t));//发回服务器处理
    }
    return 0;
}
