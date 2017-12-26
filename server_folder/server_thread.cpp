#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
using namespace std;

#define portnumber 3333  
    
void * read_msg(void *arg)  
{  
    int fd = *((int *)arg);  
    int nread = 0;  
    char buffer[1024];  
    
    while((nread = read(fd,buffer,sizeof(buffer))) > 0)  
    {  
        buffer[nread] = '\0';  
        printf("get client message: %s\n",buffer);  
        memset(buffer,0,sizeof(buffer));  
    }  
}  
    
void * write_msg(void * arg)  
{  
        
    int fd = *((int *)arg);  
        
    while(1)  
    {  
        write(fd,"hello",5);  
        sleep(2);  
    }  
}  
    
int main(int argc, char *argv[])  
{  
    int sockfd,new_fd;  
    struct sockaddr_in server_addr;  
    struct sockaddr_in client_addr;  
    int sin_size;  
    int nbytes;  
    char buffer[1024];  
    
    pthread_t id;  
    
    
    /* 服务器端开始建立sockfd描述符 */  
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:IPV4;SOCK_STREAM:TCP  
    {  
        fprintf(stderr,"Socket error:%s\n\a",strerror(errno));  
        exit(1);  
    }  
    
    /* 服务器端填充 sockaddr结构 */  
    bzero(&server_addr,sizeof(struct sockaddr_in)); // 初始化,置0  
    server_addr.sin_family=AF_INET;                 // Internet  
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);  // (将本机器上的long数据转化为网络上的long数据)和任何主机通信  //INADDR_ANY 表示可以接收任意IP地址的数据，即绑定到所有的IP  
    //server_addr.sin_addr.s_addr=inet_addr("192.168.1.1");  //用于绑定到一个固定IP,inet_addr用于把数字加格式的ip转化为整形ip  
    server_addr.sin_port=htons(portnumber);         // (将本机器上的short数据转化为网络上的short数据)端口号  
    
    /* 捆绑sockfd描述符到IP地址 */  
    if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)  
    {  
        fprintf(stderr,"Bind error:%s\n\a",strerror(errno));  
        exit(1);  
    }  
    
    /* 设置允许连接的最大客户端数 */  
    if(listen(sockfd,5)==-1)  
    {  
        fprintf(stderr,"Listen error:%s\n\a",strerror(errno));  
        exit(1);  
    }  
    
    while(1)  
    {  
        /* 服务器阻塞,直到客户程序建立连接 */  
        sin_size=sizeof(struct sockaddr_in);  
        printf("accepting!\n");  
        if((new_fd=accept(sockfd,(struct sockaddr *)(&client_addr),&sin_size))==-1)  
        {  
            fprintf(stderr,"Accept error:%s\n\a",strerror(errno));  
            exit(1);  
        }  
        fprintf(stderr,"Server get connection from %s\n",inet_ntoa(client_addr.sin_addr)); // 将网络地址转换成.字符串  
        
        std::thread t1(read_msg, &new_fd);
        std::thread t2(write_msg, &new_fd);
        t1.join();
        t2.join();
    }  
    
    /* 结束通讯 */  
    close(sockfd);  
    exit(0);  
}  