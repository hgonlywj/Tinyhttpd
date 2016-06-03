gcc -g -W -o httpd httpd.c -lpthread 


gcc -g -W -o simpleclient simpleclient.c 

gcc -g -W -o test test.c

// 如何使用cgi
http://blog.chinaunix.net/xmlrpc.php?r=blog/article&uid=20546732&id=359966


http://127.0.0.1:4000/



lsof -i:4000            查看4000端口被谁占用 
ps -efl | grep 4000     查看4000端口



#include<winsock2.h>
#include<stdio.h>
#pragmacomment(lib,"ws2_32.lib")
void main()
{
    WSADATA wsaData;
    SOCKET sockServer;
    SOCKADDR_IN addrServer;
    SOCKET sockClient;
    SOCKADDR_IN addrClient;
    WSAStartup(MAKEWORD(2,2),&wsaData);
    sockServer=socket(AF_INET,SOCK_STREAM,0);
    addrServer.sin_addr.S_un.S_addr=htonl(INADDR_ANY);//INADDR_ANY表示任何IP
    addrServer.sin_family=AF_INET;
    addrServer.sin_port=htons(6000);//绑定端口6000
    bind(sockServer,(SOCKADDR*)&addrServer,sizeof(SOCKADDR));
     
    //Listen监听端
    listen(sockServer,5);//5为等待连接数目
    printf("服务器已启动:\n监听中...\n");
    int len=sizeof(SOCKADDR);
    charsendBuf[100];//发送至客户端的字符串
    charrecvBuf[100];//接受客户端返回的字符串
     
    //会阻塞进程，直到有客户端连接上来为止
    sockClient=accept(sockServer,(SOCKADDR*)&addrClient,&len);
    //接收并打印客户端数据
    recv(sockClient,recvBuf,100,0);
    printf("%s\n",recvBuf);
     
    //关闭socket
    closesocket(sockClient);
    WSACleanup();
}