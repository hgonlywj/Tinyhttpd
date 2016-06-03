/*
    好久不写socket了，就系统的做个注释。
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
 
int main()
{
    int sockfd;
    int len;
    struct sockaddr_in address;     // 定义socket的结构体
    int result;                     // 返回结果
    char ch = 'A';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);   // 创建一个socket对象
                                                
    if (sockfd == -1) {
        printf("socket create error ... ");
        return -1;
    }
    address.sin_family = AF_INET;                           // AF_INET决定了要用ipv4地址
    address.sin_addr.s_addr = inet_addr("127.0.0.1");       // 目标IP
    address.sin_port = htons(4000);                         // 绑定端口
    len = sizeof(address);

    // 连接到服务器
    result = connect(sockfd, (struct sockaddr *)&address, len);     // 0 是成功
    if (result == -1)
    {
        perror("oops: client1");
        return -1;
    }
    printf("result = %d\n", result);


/**
 *  这里的测试是一个socket的协议，而不是一个http协议的测试
 */
    int retval = write(sockfd, &ch, 1);
    if (retval <= 0) {
        printf("write function error \n");
        return -1;
    }
    read(sockfd, &ch, 1);
    
    printf("char from server = %s\n", buf);
    close(sockfd);
    
    return 0;
}
