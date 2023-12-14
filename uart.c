#include "uart.h"

/**
 * 串口初始化操作
 * @param _tUart 串口对象结构体
 * @param device 表示串口终端的设备节点
*/
int uart_init(tagUart_T *_tUart,const char *device)
{
    /* 打开串口终端 */
    _tUart->fd = open(device,O_RDWR|O_NONBLOCK);    //以读写非阻塞方式打开
    if(_tUart->fd < 0)
    {
        fprintf(stderr,"open error:%s: %s\n",device,strerror(errno));
        return -1;
    }

    /* 获取串口当前的配置参数 */
    if(tcgetattr(_tUart->fd,&_tUart->old_cfg) < 0)
    {
        fprintf(stderr,"tcgetattr error:%s\n",strerror(errno));
        close(_tUart->fd);
        return -1;
    }

    return 0;
}

/**
 * 串口配置
 * @param _tUart 串口对象结构体
 * @param cfg 指向一个 Uart_cfg_t 结构体对象
*/
int uart_cfg(tagUart_T *_tUart,const Uart_cfg_t *cfg)
{
    struct termios new_cfg = {0};   //将新的配置对象清零
    speed_t speed;

    /* 设置为原始模式 */
    cfmakeraw(&new_cfg);

    /* 使能接收 */
    new_cfg.c_cflag |= CREAD;

    /* 设置波特率 */
    switch (cfg->baudrate) {
    case 1200: speed = B1200;
    break;
    case 1800: speed = B1800;
    break;
    case 2400: speed = B2400;
    break;
    case 4800: speed = B4800;
    break;
    case 9600: speed = B9600;
    break;
    case 19200: speed = B19200;
    break;
    case 38400: speed = B38400;
    break;
    case 57600: speed = B57600;
    break;
    case 115200: speed = B115200;
    break;
    case 230400: speed = B230400;
    break;
    case 460800: speed = B460800;
    break;
    case 500000: speed = B500000;
    break;
    default: //默认配置为 115200
    speed = B115200;
    printf("default baud rate: 115200\n");
    break;
    }

    if(cfsetspeed(&new_cfg,speed) < 0)
    {
        fprintf(stderr,"cfsetspeed error:%s\n",strerror(errno));
        return -1;
    }

    /* 设置数据位大小 */
    new_cfg.c_cflag &= ~CSIZE;  //将数据位相关的比特位清零
    
    switch(cfg->dbit)
    {
        case 5:
            new_cfg.c_cflag |= CS5;
            break;
        case 6:
            new_cfg.c_cflag |= CS6;
            break;
        case 7:
            new_cfg.c_cflag |= CS7;
            break;
        case 8:
            new_cfg.c_cflag |= CS8;
            break;
        default:
            new_cfg.c_cflag |= CS8;
            printf("default data bit size:8\n");
            break;
    }

    /* 设置奇偶校验 */
    switch(cfg->parity)
    {
        case 'N':
            new_cfg.c_cflag &= ~PARENB;
            new_cfg.c_iflag &= ~INPCK;
            break;
        case 'O':
            new_cfg.c_cflag |= (PARODD | PARENB);
            new_cfg.c_iflag |= INPCK;
            break;
        case 'E':
            new_cfg.c_cflag |= PARENB;
            new_cfg.c_cflag &= ~PARODD;/* 清除 PARODD 标志，配置为偶校验 */
            new_cfg.c_iflag |= INPCK;
            break;
        default:
            new_cfg.c_cflag &= ~PARENB;
            new_cfg.c_iflag &= ~INPCK;
            printf("default parity:N\n");
            break;
    }

    /* 设置停止位 */
    switch(cfg->sbit)
    {
        case 1:
            new_cfg.c_cflag &= ~CSTOPB;
            break;
        case 2:
            new_cfg.c_cflag |= CSTOPB;
            break;
        default:
            new_cfg.c_cflag &= ~CSTOPB;
            printf("default stop bit size:1\n");
            break;
    }

    /* 将MIN和TIME设置为0 */
    new_cfg.c_cc[VTIME] = 0;
    new_cfg.c_cc[VMIN] = 0;

    /* 清空缓冲区 */
    if(tcflush(_tUart->fd,TCIOFLUSH) < 0)
    {
        fprintf(stderr,"tcflush error:%s\n",strerror(errno));
        return -1;
    }

    /* 写入配置、配置生效 */
    if(tcsetattr(_tUart->fd,TCSANOW,&new_cfg) < 0)
    {
        fprintf(stderr,"tcsetattr error:%s\n",strerror(errno));
        return -1;
    }

    return 0;
}
