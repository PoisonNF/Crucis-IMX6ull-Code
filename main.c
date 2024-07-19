#include "main.h"
#include "uart.h"

tagUart_T Uart3;        //GPS数据串口
tagUart_T Uart2;        //STM32数据串口

char StartClpt[] = "StartClpt";     //i.mx6ull板启动完成提示

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
            write(Uart2.fd,GNRMC,ret);        //暂时放外面，后续通过'A'进行发送

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
 * 信号处理函数，当串口有数据可以读时，会跳转该函数执行
*/
static void Uart2_handler(int sig,siginfo_t *info,void *context)
{
    int ret;

    unsigned char buf[20] = {0};   //数据暂存

    if((SIGRTMIN+1) != sig)
        return;
    /* 判断串口是否有数据可读 */
    if (POLL_IN == info->si_code) {
        ret = read(Uart2.fd, buf, 20); 
        write(Uart2.fd,buf,ret);
        printf("%s\n",buf);
    }
}

/**
 * 异步Uart3I/O初始化函数
*/
static void async_U3io_init(void)
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

/**
 * 异步Uart2I/O初始化函数
*/
static void async_U2io_init(void)
{
    struct sigaction sigatn;
    int flag;

    /* 使能异步I/O */
    flag = fcntl(Uart2.fd,F_GETFL);
    flag |= O_ASYNC;
    fcntl(Uart2.fd,F_SETFL,flag);

    /* 设置异步I/O的所有者 */
    fcntl(Uart2.fd,F_SETOWN,getpid());

    /* 指定实时信号SIGRTMIN+1作为异步I/O的通知信号 */
    fcntl(Uart2.fd,F_SETSIG,SIGRTMIN+1);

    /* 为实时信号SIGRTMIN+1注册信号处理函数 */
    sigatn.sa_sigaction = Uart2_handler; //当串口有数据可读时，会跳转到io_handler函数
    sigatn.sa_flags = SA_SIGINFO;
    sigemptyset(&sigatn.sa_mask);
    sigaction(SIGRTMIN+1,&sigatn,NULL);
}


int main(int argc,char *argv[])
{
    Uart_cfg_t cfg = {0};
    pid_t ChildPid;     //子进程的PID

    cfg.baudrate = 38400;
    cfg.dbit = 8;
    cfg.parity = 'N';
    cfg.sbit = 1;

    /* 串口初始化 */
    if(uart_init(&Uart3,"/dev/ttymxc2"))
        exit(EXIT_FAILURE);

    if(uart_init(&Uart2,"/dev/ttymxc1"))
        exit(EXIT_FAILURE);

    /* 串口配置 */
    if(uart_cfg(&Uart3,&cfg))
    {
        tcsetattr(Uart3.fd,TCSANOW,&Uart3.old_cfg); //恢复之前的配置
        close(Uart3.fd);
        exit(EXIT_FAILURE);
    }

    cfg.baudrate = 115200;
    if(uart_cfg(&Uart2,&cfg))
    {
        tcsetattr(Uart2.fd,TCSANOW,&Uart2.old_cfg); //恢复之前的配置
        close(Uart2.fd);
        exit(EXIT_FAILURE);
    }

    //async_U2io_init();  //串口2异步方式初始化
    //async_U3io_init();  //串口3异步方式初始化

    printf("Wait read\n");
    // for(;;)
    // {
    //     sleep(1);
    // } 

    //告知STM32已经启动完成
    write(Uart2.fd,StartClpt,sizeof(StartClpt));
    printf("I.MX6ULL Start\n");

    switch(ChildPid = fork())
    {
        case -1:    /* 出错 */
            perror("fork");
        case 0:     /* 子进程 */
            printf("ChildPid = %d\n",getpid());
            async_U2io_init();  //串口2异步方式初始化
            while(1)
            {
                sleep(1);       //进程挂起，可被信号打断
            }
            //_exit(EXIT_SUCCESS);
        default:    /* 父进程 */
            printf("parentPid = %d\n",getpid());
            async_U3io_init();  //串口3异步方式初始化
            while(1)
            {
                sleep(1);       //进程挂起，可被信号打断
            }
            //wait(NULL);
    }

    /* 退出 */
    tcsetattr(Uart3.fd,TCSANOW,&Uart3.old_cfg);
    close(Uart3.fd);

    tcsetattr(Uart2.fd,TCSANOW,&Uart2.old_cfg);
    close(Uart2.fd);
    exit(EXIT_SUCCESS);
}