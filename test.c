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


#define ISspace(x) isspace((int)(x))

int main() {

    // pthread_t newthread;
    
    // printf("%ls \n", newthread);

    printf("%d\n", ISspace(' '));
    printf("%d\n", ISspace('\r'));
    printf("%d\n", ISspace('\n'));

    //获取当前日历时间  
    time_t t;  
    t = time(&t);  
    char buf[1024];

    // printf("the current time of seconds:%ld, string is:%s\n", t, ctime(&t));  
    sprintf(buf, "Dtae: %s\n", ctime(&t));  
    printf("%s\n", buf);

    return 0;
}