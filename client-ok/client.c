#include <my_head.h>
#include "program.h"
//客户机
pthread_t tidfan=0;
pthread_arg fan_arg={0};
pthread_arg my_arg1={0};
pthread_mutex_t mylock;
Limitset_t mylimit={//阈值
    .tempup=200.0,
    .tempdown=-100.0,
    .humup=200.0,
    .humdown=-100.0,
    .iullup=10000.0,
    .iulldown=-100.0
};
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
void *pthread_fan(void* arg){
    pthread_arg my_argfan=*((pthread_arg *)arg);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    int which=0;
    while(1){
        ioctl(my_argfan.fd5, FAN_ON, &which);
        usleep(300000);
    }
}
void *pthread_fun(void* arg){//阈值
    pthread_detach(pthread_self());
    pthread_arg my_arg=*((pthread_arg *)arg);
    int which=0;
    int sun,temp,hum;
    float my_sun,my_temp,my_hum;
    while(1){
        ioctl(my_arg.fd1, GET_SUN, &sun);
        ioctl(my_arg.fd2, GET_TMP, &temp);
        ioctl(my_arg.fd2, GET_HUM, &hum);
        my_sun = 0.35 * sun;//lx 370
        my_temp = 175.72 * temp / 65536 - 46.85;//℃ 26
        my_hum = 125.0 * hum / 65536.0 - 6;//% 29
        if(mylimit.iullup<my_sun){//光照
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.ledstate=0;
            led_off_all(my_arg.fd3);
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.iulldown>my_sun){
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.ledstate=1;
            led_on_all(my_arg.fd3);
            pthread_mutex_unlock(&mylock);
        }

        if(mylimit.tempup<my_temp){//温度
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.fanstate=0;
            if(tidfan>0){
                pthread_cancel(tidfan);
                pthread_join(tidfan,NULL);
                tidfan=0;
                usleep(200000);
                ioctl(my_arg.fd5, FAN_OFF, &which);
            }
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.tempdown>my_temp){
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.fanstate=1;
            if(tidfan<=0){
                pthread_create(&tidfan,NULL,pthread_fan,(void*)&fan_arg);
            }
            pthread_mutex_unlock(&mylock);
        }
        
        if(mylimit.humup<my_hum){//湿度
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.humstate=0;
            ioctl(my_arg.fd4, BEEP_OFF, &which);
            pthread_mutex_unlock(&mylock);
        }else if(mylimit.humdown>my_hum){
            pthread_mutex_lock(&mylock);
            my_arg.tmpMsg->env.humstate=1;
            ioctl(my_arg.fd4, BEEP_ON, &which);
            pthread_mutex_unlock(&mylock);
        }
        usleep(600000);
    }
}
int main(int argc, const char *argv[]){
    int fd1,fd2,fd3,fd4,fd5;
    int sun,tmp,hum;
    float lux,rtmp,rhum;
    int which=0;
    char ip_addr[32]={0};
    char buff[64]={0};
    char id[20]={0};
    char password[20]={0};
    int port_num,fd=0;
    pthread_t tid=0;
    Msg_t myMsg={0};
    Msg_t tmpMsg={0};
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
    fan_arg.fd5=fd5;
    my_arg1.fd1=fd1;
    my_arg1.fd2=fd2;
    my_arg1.fd3=fd3;
    my_arg1.fd4=fd4;
    my_arg1.fd5=fd5;
    my_arg1.myMsg=&myMsg;
    my_arg1.tmpMsg=&tmpMsg;
    pthread_create(&tid,NULL,pthread_fun,(void*)&my_arg1);
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
    printf("请输入用户名：");
    scanf("%s",id);
    if(connect(fd,(const struct sockaddr*)&server_msg_st,sizeof(server_msg_st))==-1){
        perror("connect error");
        return -1;
    }
    printf("connect success\n");
    write(fd,id,sizeof(id));
    while(1){
        read(fd,&myMsg,sizeof(Msg_t));
        ioctl(fd1, GET_SUN, &sun);
        ioctl(fd2, GET_TMP, &tmp);
        ioctl(fd2, GET_HUM, &hum);
        myMsg.env.sunvalue = 0.35 * sun;//370 lux
        myMsg.env.tempvalue = 175.72 * tmp / 65536 - 46.85;//26 ℃
        myMsg.env.humvalue = 125 * hum / 65536.0 - 6;//29 %
        switch(myMsg.recvtype){
            case 1://设备控制
                if(myMsg.dev.led==1){
                    if(myMsg.env.sunvalue>mylimit.iullup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.ledstate=1;
                    led_on_all(fd3);
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(myMsg.env.sunvalue<mylimit.iulldown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.ledstate=0;
                    led_off_all(fd3);
                    pthread_mutex_unlock(&mylock);
                }
                if(myMsg.dev.beep==1){
                    if(myMsg.env.humvalue>mylimit.humup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.humstate=1;
                    ioctl(fd4, BEEP_ON, &which);
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(myMsg.env.humvalue<mylimit.humdown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.humstate=0;
                    ioctl(fd4, BEEP_OFF, &which);
                    pthread_mutex_unlock(&mylock);
                }
                if(myMsg.dev.fan==1){
                    if(myMsg.env.tempvalue>mylimit.tempup){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.fanstate=1;
                    if(tidfan<=0){
                        pthread_create(&tidfan,NULL,pthread_fan,(void*)&fan_arg);
                    }
                    pthread_mutex_unlock(&mylock);
                }else{
                    if(myMsg.env.tempvalue<mylimit.tempdown){
                        myMsg.flags=1;
                    }
                    pthread_mutex_lock(&mylock);
                    tmpMsg.env.fanstate=0;
                    if(tidfan>0){
                        pthread_cancel(tidfan);
                        pthread_join(tidfan,NULL);
                        tidfan=0;
                        ioctl(fd5, FAN_OFF, &which);
                        usleep(200000);
                        ioctl(fd5, FAN_OFF, &which);
                    }
                    pthread_mutex_unlock(&mylock);
                }
                break;
            case 3://阈值设置,
                if(myMsg.limit.tempup>-0.1 && myMsg.limit.tempup<100.1)
                    mylimit.tempup=myMsg.limit.tempup;
                if(myMsg.limit.tempdown>-0.1 && myMsg.limit.tempdown<mylimit.tempup)
                    mylimit.tempdown=myMsg.limit.tempdown;
                if(myMsg.limit.humup>-0.1 && myMsg.limit.humup<100.1)
                    mylimit.humup=myMsg.limit.humup;
                if(myMsg.limit.humdown>-0.1 && myMsg.limit.humdown<mylimit.humup)
                    mylimit.humdown=myMsg.limit.humdown;
                if(myMsg.limit.iullup>-0.1 && myMsg.limit.iullup<10000.1)
                    mylimit.iullup=myMsg.limit.iullup;
                if(myMsg.limit.iulldown>-0.1 && myMsg.limit.iulldown<mylimit.iullup)
                    mylimit.iulldown=myMsg.limit.iulldown;
                break;
        }
        myMsg.env.ledstate=tmpMsg.env.ledstate;
        myMsg.env.fanstate=tmpMsg.env.fanstate;
        myMsg.env.humstate=tmpMsg.env.humstate;
        write(fd,&myMsg,sizeof(Msg_t));//发回服务器处理
    }
    return 0;
}
