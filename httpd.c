#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>  

#define ISspace(x) isspace((int)(x))    // ‘ ’ '\r' '\n' --> 1 空格，回车，换行
#define REMEMBER_SERVER "Server: remember the age of innocence /0.0.1\r\n"  // Response Header of Server
#define STDIN   0
#define STDOUT  1
#define STDERR  2

void accept_request(void *);        // 解析请求
void bad_request(int);              // 404
void cat(int, FILE *);              // 通过tcp，发送文件
void cannot_execute(int);           
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
 int get_line(int, char *, int);     // 读取套接字的一行,把回车换行等情况都统一为换行符结束
void headers(int, const char *, char *);
void not_found(int);
void server_file(int, const char *);
 int startup(u_short *);
void unimplemented(int);

/**********************************************************************/
/* 
 * 参数：端口的ID
 *  
 * 处理从套接字上监听到的一个 HTTP 请求,在这里可以很大一部分地体现服务器处理请求流程
 */
/**********************************************************************/
void accept_request(void *arg)
{
    int client = *(int*)arg;    // 获取客户端的id
    char buf[1024];
    size_t numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;                 // 通过文件名filename获取文件信息，并保存在buf所指的结构体stat中 
    int cgi = 0;                    // cgi 标示，如果是cgi，就为1 
    char *query_string = NULL;      // 参数
    

    // // 打印HTTP头文件
    // numchars = get_line(client, buf, sizeof(buf));
    // int numbers = 0;
    // while (numchars > 0) {
    //     printf("%d  =  %s\n", ++numbers, buf);
    //     numchars = get_line(client, buf, sizeof(buf));
    // }


    numchars = get_line(client, buf, sizeof(buf));          // 获取 一行数据 以 \n \r 结尾的字符串
    // printf("buf = %s\n", buf);                           // buf = GET / HTTP/1.1

    i = 0; j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))    // 以 \r \n 作为结尾
    {
        method[i] = buf[i];
        i++;
    }
    j = i;
    method[i] = '\0';                                       // 字符串补 \0

    printf("method －－－－－ %s\n", method);

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))    // 这里只判断了get 和 post 
    {
        unimplemented(client);      // 返回，并没有这个方法
        return;
    }

    if (strcasecmp(method, "POST") == 0) {      // post 方法  strcasecmp忽略大小写比较字符串
        cgi = 1;
    }

    i = 0;
    while (ISspace(buf[j]) && (j < numchars)) {
        j++;
    }

    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)     // GET方式，可能会传入参数
    {
        query_string = url;
        printf("url = %s\n", url);
        // 找到？ 将GET后面的变量都读出来 
        while ((*query_string != '?') && (*query_string != '\0')) {
            query_string++;
        }

        if (*query_string == '?') {
            printf("query_string ＝ %s\n\n", query_string);
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "App%s", url);
    if (path[strlen(path) - 1] == '/') {
        strcat(path, url);
    }


    // stat获取文件信息，放入st中；执行成功则返回0，失败返回-1
    if (stat(path, &st) == -1) {    // 失败
        while ((numchars > 0) && strcmp("\n", buf)) {   /* read & discard headers */ 
            numchars = get_line(client, buf, sizeof(buf));
        }

        not_found(client);
    }
    else {      // 成功 

        /*
            S_IFMT : 是一个掩码，它的值是017000(八进制)
            S_IFDIR : 是一个文件夹
         */
        if ((st.st_mode & S_IFMT) == S_IFDIR)  { // st_mode 文件的类型和存取的权限  
            strcat(path, url);
        }    

        // switch (path) {
        //     case :
        // }
        // printf("cgi = %d\n", cgi);

        server_file(client, path);

        // if ((st.st_mode & S_IXUSR) ||       // owner has execute permission
        //     (st.st_mode & S_IXGRP) ||       // group has read permission
        //     (st.st_mode & S_IXOTH))         // others have execute permission
        // {
        //     cgi = 1;
        // }


    //     // if (!cgi) {
    //     //     server_file(client, path);
    //     // }
    //     // else {
    //     //     printf("accept_request —— path = %s, method = %s, query_string = %s\n", 
    //     //             path, method, query_string);

    //     //     execute_cgi(client, path, method, query_string);
    //     // }
    }

    close(client);
}

/**********************************************************************/
/* 
 *  请求失败
 */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* 
 *
 * 读取服务器上某个文件写到 socket 套接字
 * 发送给客户端
 */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    // printf("%s\n", buf);
    while (!feof(resource))     // 循环读取文件到结束
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
        // printf("%s\n", buf);

    }
    // printf("cat is over ... finish!\n");
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error.
 *  
 * 把错误信息写到 perror 并退出。
 */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc); 
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, 
                 const char *path,
                 const char *method, 
                 const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        {
            numchars = get_line(client, buf, sizeof(buf));
            // printf("numchars = %d\n", numchars);
            // printf("buf = %s\n", buf);
        }    
    }
    else if (strcasecmp(method, "POST") == 0) /*POST*/
    {
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        if (content_length == -1) {
            bad_request(client);
            return;
        }
    }
    else/*HEAD or other*/
    {

    }

    if (pipe(cgi_output) < 0) {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }
    if ( (pid = fork()) < 0 ) {
        cannot_execute(client);
        return;
    }

    printf("execute_cgi pid = %d\n", pid);

    
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    printf("execute_cgi buf ＝ %s\n", buf);
    

    if (pid == 0)               /* child: CGI script */
    {
        printf("child: CGI script\n");
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);

        printf("cgi_output[1] %d\n", cgi_output[1]);
        printf("cgi_input[0] %d\n", cgi_input[0]);

        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);

        printf("meth_env = %s\n", meth_env);

        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
            printf("query_env = %s\n", query_env);
        }
        else {   /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, NULL);
        exit(0);

    } else {                            /* parent */

        printf("parent fork\n");
        close(cgi_output[1]);
        close(cgi_input[0]);

        if (strcasecmp(method, "POST") == 0) {            
            for (i = 0; i < content_length; i++) {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }

        // cgi_output 发回浏览器
        printf("CGI cgi_output[0] %d%d 发回客户端\n", cgi_output[0], cgi_output[1]);
        printf("client = %d\n", client);

        while (read(cgi_output[0], &c, 1) > 0) {
            send(client, &c, 1, 0);     // 浏览器发数据
            printf("%c", c);
        }
        
        printf("send something ...\n");


        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* 
 *
 * 取套接字的一行，把回车换行等情况都统一为换行符结束 
 */

/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    // printf("get_line ———— buf=%s, length=%d \n", buf, i);
    return(i);
}

/**********************************************************************/
/* 
 * HTTP的头部规则，
 * 按规则写入
 * 浏览器就能识别
 * 并发送给浏览器
 */
/**********************************************************************/
void headers(int client, const char *filename, char *content_type)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    time_t t;        //获取当前日历时间
    t = time(&t);  

    // HTTP/1.1 ［协议］ 200［状态码］ OK［原因短语］
    strcpy(buf, "HTTP/1.1 200 OK\r\n");     
    send(client, buf, strlen(buf), 0);
    printf("%s\n", buf);

    // 响应首部字段
    strcpy(buf, REMEMBER_SERVER);           // Server: h.g. remember the age of innocence /0.0.1
    send(client, buf, strlen(buf), 0);
    // sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n");     // application/javascript; charset=utf-8
    send(client, content_type, strlen(content_type), 0);
    printf("%s\n", content_type);
    sprintf(buf, "Date: %s h.g.\r\n", ctime(&t));   
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Access-Control-Allow-Headers:X-Requested-With\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Expires: %s h.g.\r\n", ctime(&t));
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Last-Modified: %s h.g.\r\n", ctime(&t));
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Vary:Accept-Encoding\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    // 响应头结束

    // 必须按照上面的规则发送，才能完成HTTP的认证
    // 发送下面这个和不发送头，是一样的。
    // 无法在浏览器的网络中查看到 response header

    // char test[1024] = "HTTP/1.0 200 OK\r\n 
    //                    Server: remember the age of innocence /1.0.0\r\n 
    //                    Content-Type: text/html\r\n 
    //                    \r\n";
    // send(client, test, strlen(test), 0);

}

/**********************************************************************/
/*  
*  主要处理找不到请求的文件时的情况   404
*/
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, REMEMBER_SERVER);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<html><title>not font</title>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<body><P>找不到文件\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<p>请求的资源不存在\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</body></html>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* 
 *  参数 发送的接口 和 文件指针
 * 
 *  调用 cat 把服务器文件返回给浏览器
 */
/**********************************************************************/
void server_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; 
    buf[1] = '\0';
    
    while ((numchars > 0) && strcmp("\n", buf))     /* read & discard headers */
    {
        numchars = get_line(client, buf, sizeof(buf));
    }   

    resource = fopen(filename, "r");                // 打开文件
    // printf("filename = %s\n", filename);         // 打印文件名
    
    char fileExtension[10];                         // 保存文件后缀名
    int length = strlen(filename), m = length, n = 0;
    while (m) {
        if (filename[m] == '.') {
            break;
        }
        m--;
    }
    while (!ISspace(filename[m]) && (m < length)) {
        fileExtension[n++] = filename[++m];
    }
    fileExtension[n] = '\0';
    printf("fileExtension = %s\n", fileExtension);  

    char content_type[100]; 
    if (0 == strcasecmp(fileExtension, "html")) {
        sprintf(content_type, "Content-Type: text/html\r\n");
    } else if (0 == strcasecmp(fileExtension, "js")) {
        sprintf(content_type, "Content-Type: application/x-javascript\r\n");
    } else if (0 == (strcasecmp(fileExtension, "css"))) {
        sprintf(content_type, "Content-Type: text/css\r\n");
    } else if (0 == (strcasecmp(fileExtension, "jpg"))) {
        sprintf(content_type, "Content-Type: image/jpeg\r\n");
    } else {
        sprintf(content_type, "Content-Type: ..... \r\n");
    }

    if (resource == NULL) {
        not_found(client);              // 主要处理找不到请求的文件时的情况
    }
    else
    {
        headers(client, filename, content_type);      // 把HTTP响应的头部写到套接字,http 的头部规则
        cat(client, resource);                        // 读取服务器上某个文件写到 socket 套接字。
    }

    fclose(resource);                   // 关闭文件
}

/**********************************************************************/
/* 
 * 初始化 httpd 服务，包括建立套接字，绑定端口，进行监听等。
 * 
 * 参数：端口 
 * 返回值是一个socket 套接字
 */
/**********************************************************************/
int startup(u_short *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);    // 创建一个socket
    if (httpd == -1) {                       // 失败 INVALID_SOCKET（－1） 
        error_die("socket");
    }
    memset(&name, 0, sizeof(name));             // 清空这段缓存
    name.sin_family = AF_INET;                  // 地址家族的格式
    name.sin_port = htons(*port);               // 绑定 端口号
    name.sin_addr.s_addr = htonl(INADDR_ANY);   // INADDR_ANY表示任何IP  htonl 示将32位的主机字节顺序转化为32位的网络字节顺序

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0) {      // socket 绑定 成功返回0
        error_die("bind");
    }
    
    printf("----绑定端口-----prot----- %d\n", *port);

    if (*port == 0)  /* 如果端口是0， 就动态分配一个 */
    {
        socklen_t namelen = sizeof(name);       // socklen_t 数据类型
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1) {
            error_die("getsockname");
        }

        *port = ntohs(name.sin_port);
        printf("----更改端口-----prot----- %d\n", *port);
    }

    if (listen(httpd, 5) < 0) {
        error_die("listen");
    }

    return(httpd);
}

/**********************************************************************/
/*　
 * 返回给浏览器表明收到的 HTTP 请求所用的 method 不被支持
 */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, REMEMBER_SERVER);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<html>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<head><h1>Method Not Implemented</h1></head>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<body><p>HTTP request method not supported.</p></body>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</html>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)
{
    int server_sock = -1;
    u_short port = 8080;
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t  client_name_len = sizeof(client_name);
    pthread_t newthread;        // 指向线程标识符的指针
    server_sock = startup(&port);    // 初始化
    printf("纪念白衣飘飘的年代 ... httpd running on port %d\n", server_sock);

    while (1)
    {
        //  accept 是阻塞进程，
        //  直到有客户端请求链接，建立好通信，就回返回一个新的套接字 client_name
        client_sock = accept(server_sock, 
                             (struct sockaddr *)&client_name, 
                             &client_name_len);

        if (client_sock == -1) {
            error_die("accept");
        }

        // 直接执下面的函数也可以
        // accept_request((void *)&client_sock); 

        // pthread_create 创建一个线程,成功返回 0
        int rpc = pthread_create(&newthread,                // 指向线程标识符的指针
                                 NULL,                      // 指向线程的类型
                                 (void *)accept_request,    // 线程运行函数的起始地址
                                 (void *)&client_sock);     // 运行函数的参数,就是 --->> accept_request(client_sock)

        if (rpc != 0)   {                                   // 不为0，就是失败
            close(server_sock);                             // 关闭链接
            printf("pthread_create");                       // 打印错误
                                                            
            return -1;
        }
    }

    // 关闭连接
    close(server_sock);

    return(0);
}