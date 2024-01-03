#ifndef __SERVER_H__
#define __SERVER_H__
#include <my_head.h>
//服务器

#define N 100

typedef struct{
    int fd;
    int epfd;
    char ip[32];
    sqlite3 *db;
    int msqid;
}pthread_arg;

typedef struct{
     float tempvalue;
     float humvalue; 
     float sunvalue;
     float volvalue;
     char ledstate;
     char fanstate;
     char humstate;   
} Envdata_t;

typedef struct{
    float tempup;
    float tempdown;
    float humup;
    float humdown;
    float iullup;
    float iulldown;
} Limitset_t;

typedef struct{
     char led;
     char fan;
     char hume;
     char beep;
} Devctrl_t; 

typedef struct{
	long msgtype;
	long recvtype;//等待消息类型 1设备控制 2网页发环境数据 3阈值设置 4网页收环境数据
	char userinfo[20];//上位机用户信息字段
	char flags;
	Envdata_t env;//环境数据
	Limitset_t limit;//阈值设置
	Devctrl_t dev;//设备控制
} Msg_t;

void *pthread_fun(void* arg);


#endif