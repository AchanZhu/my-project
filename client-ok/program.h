#ifndef __PROGRAM_H__
#define __PROGRAM_H__
//客户机

void led_on_all(int fd);
void led_off_all(int fd);
void led_on(int fd, int which);
void led_off(int fd, int which);
#define BEEP_ON _IOW('b',0,int)
#define BEEP_OFF _IOW('b',1,int)
#define FAN_ON _IOW('c',0,int)
#define FAN_OFF _IOW('c',1,int)

#define LED_ON _IOW('a',0,int)
#define LED_OFF _IOW('a',1,int)
#define TMP_ADDR 0xe3
#define HUM_ADDR 0xe5
#define SUN_HIGH_ADDR 0x0d
#define SUN_LOW_ADDR 0x0c
#define GET_TMP _IOR('g',0,int)
#define GET_HUM _IOR('g',1,int)
#define GET_SUN _IOR('g',2,int)
#define SYS_CONF 0x0
#define INT_STAT 0x01
#define CLR_MANG 0x02

enum leds{
    LED1,
    LED2,
    LED3,
    LED4,
    LED5,
    LED6
};
enum leds_status{
    OFF,
    ON
};

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
	long long msgtype;
	long long recvtype;//等待消息类型 1设备控制 2网页发环境数据 3阈值设置 4网页收环境数据
	char userinfo[20];//上位机用户信息字段
	char flags;//用于标识下位机是否存在   1存在，0不存在
	Envdata_t env;//环境数据
	Limitset_t limit;//阈值设置
	Devctrl_t dev;//设备控制
} Msg_t;

typedef struct{
    int fd1;
    int fd2;
    int fd3;
    int fd4;
    int fd5;
    Msg_t *myMsg;
    Msg_t *tmpMsg;
}pthread_arg;

void *pthread_fan(void* arg);
void *pthread_fun(void* arg);

#endif