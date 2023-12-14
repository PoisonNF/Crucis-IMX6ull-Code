#include "main.h"
#include "uart.h"

tagUart_T Uart3;        //读取GPS数据

/**
 * 打印帮助信息
*/
static void show_help(const char *app)
{
printf("Usage: %s [选项]\n"\
        "\n 必选选项:\n"\
        " --dev=DEVICE 指定串口终端设备名称, 譬如--dev=/dev/ttymxc2\n"\
        " --type=TYPE 指定操作类型, 读串口还是写串口, 譬如--type=read(read 表示读、write 表示\
        写、其它值无效)\n"\
        "\n 可选选项:\n"\
        " --brate=SPEED 指定串口波特率, 譬如--brate=115200\n"
        " --dbit=SIZE 指定串口数据位个数, 譬如--dbit=8(可取值为: 5/6/7/8)\n"
        " --parity=PARITY 指定串口奇偶校验方式, 譬如--parity=N(N 表示无校验、O 表示奇校验、E 表\
        示偶校验)\n"\
        " --sbit=SIZE 指定串口停止位个数, 譬如--sbit=1(可取值为: 1/2)\n"
        " --help 查看本程序使用帮助信息\n\n", app);    
}

/**
 * 信号处理函数，当串口有数据可以读时，会跳转该函数执行
*/
static void Uart3_handler(int sig,siginfo_t *info,void *context)
{
    int ret;

    unsigned char buf[200] = {0};   //数据暂存
    char s_buf[6] = "$GNRMC";       //需要搜索的字符串
    char GNRMC[100];                //储存GNRMC数据

    char LatitudeStr[10];   //纬度字符串
    char LongitudeStr[11];  //经度字符串

    char status;        //数据状态
    double Latitude;     //纬度
    double Longitude;    //经度

    if(SIGRTMIN != sig)
        return;
    /* 判断串口是否有数据可读 */
    if (POLL_IN == info->si_code) {
        ret = read(Uart3.fd, buf, 200); 

        //printf("%.*s",ret,buf);
        //printf("num = %d\n",ret);

        /* 在接收到的数据中寻找GNRMC数据 */
        char* result = strstr(buf, s_buf);
        if(result != NULL)
        {
            memcpy(GNRMC,result,ret);
            printf("%s",GNRMC);
            //printf("%d",sizeof(test));

            if(GNRMC[18] == 'A')   //有效定位
            {
                ret = sscanf(GNRMC,"$GNRMC,%*10[^,],%c,%10s,%*1c,%11s,%*c,%*.1f,%*.1f,%*6d,,,%*4s",
                            &status,
                            &LatitudeStr,  
                            &LongitudeStr
                );
                Latitude = strtod(LatitudeStr,NULL);
                Longitude = strtod(LongitudeStr,NULL);               
                printf("%d %c %4.5f %5.5f\n",ret,status,Latitude,Longitude);
            }
        }
    }
}

/**
 * 异步I/O初始化函数
*/
static void async_io_init(void)
{
    struct sigaction sigatn;
    int flag;

    /* 使能异步I/O */
    flag = fcntl(Uart3.fd,F_GETFL);
    flag |= O_ASYNC;
    fcntl(Uart3.fd,F_SETFL,flag);

    /* 设置异步I/O的所有者 */
    fcntl(Uart3.fd,F_SETOWN,getpid());

    /* 指定实时信号SIGRTMIN作为异步I/O的通知信号 */
    fcntl(Uart3.fd,F_SETSIG,SIGRTMIN);

    /* 为实时信号SIGRTMIN注册信号处理函数 */
    sigatn.sa_sigaction = Uart3_handler; //当串口有数据可读时，会跳转到io_handler函数
    sigatn.sa_flags = SA_SIGINFO;
    sigemptyset(&sigatn.sa_mask);
    sigaction(SIGRTMIN,&sigatn,NULL);
}


int main(int argc,char *argv[])
{
    Uart_cfg_t cfg = {0};
    char *device = NULL;
    int rw_flag = -1;
    unsigned char w_buf[10] = {0x11, 0x22, 0x33, 0x44,
                0x55, 0x66, 0x77, 0x88}; //通过串口发送出去的数据
    int n;

    /* 解析出参数 */
    for(n = 1;n < argc;n++)
    {
        if (!strncmp("--dev=", argv[n], 6))
            device = &argv[n][6];
        else if (!strncmp("--brate=", argv[n], 8))
            cfg.baudrate = atoi(&argv[n][8]);
        else if (!strncmp("--dbit=", argv[n], 7))
            cfg.dbit = atoi(&argv[n][7]);
        else if (!strncmp("--parity=", argv[n], 9))
            cfg.parity = argv[n][9];
        else if (!strncmp("--sbit=", argv[n], 7))
            cfg.sbit = atoi(&argv[n][7]);
        else if (!strncmp("--type=", argv[n], 7)) {
            if (!strcmp("read", &argv[n][7]))
                rw_flag = 0; //读
            else if (!strcmp("write", &argv[n][7]))
                rw_flag = 1; //写
        }
        else if (!strcmp("--help", argv[n])) {
            show_help(argv[0]); //打印帮助信息
            exit(EXIT_SUCCESS);
        }
    }

    if(device == NULL || rw_flag == -1)
    {
        fprintf(stderr,"Error:the device and read|write type must be set!\n");
        show_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 串口初始化 */
    if(uart_init(&Uart3,device))
        exit(EXIT_FAILURE);

    /* 串口配置 */
    if(uart_cfg(&Uart3,&cfg))
    {
        tcsetattr(Uart3.fd,TCSANOW,&Uart3.old_cfg); //恢复之前的配置
        close(Uart3.fd);
        exit(EXIT_FAILURE);
    }

    /* 读写串口 */
    switch(rw_flag)
    {
        case 0: //读串口数据
            async_io_init();    //异步方式读取数据
            printf("Wait read\n");
            for(;;)
            {
                sleep(1);
            } 
            break;
        case 1:
            printf("Wait write\n");
            while(1)
            {
                write(Uart3.fd,w_buf,8);
                sleep(1);
            }
            break;
    }

    /* 退出 */
    tcsetattr(Uart3.fd,TCSANOW,&Uart3.old_cfg);
    close(Uart3.fd);
    exit(EXIT_SUCCESS);
}