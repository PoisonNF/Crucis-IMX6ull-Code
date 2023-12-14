#ifndef __UART_H
#define __UART_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>

/* 串口硬件配置结构体 */
typedef struct Uart_hardware_cfg
{
    unsigned int baudrate;  //波特率
    unsigned char dbit;     //数据位
    char parity;            //奇偶校验
    unsigned char sbit;     //停止位
}Uart_cfg_t;

/* 串口对象结构体 */
typedef struct 
{
    int fd;                 //文件描述符
    struct termios old_cfg; //老的配置
}tagUart_T;

int uart_init(tagUart_T *_tUart,const char *device);
int uart_cfg(tagUart_T *_tUart,const Uart_cfg_t *cfg);

#endif // !__UART_H