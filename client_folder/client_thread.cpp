#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>
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
        printf("get server message: %s\n",buffer);  
        memset(buffer,0,sizeof(buffer));  
        sleep(2);  
    }  
}  
    
int main(int argc, char *argv[])   
{   
    int sockfd;   
    char buffer[1024];   
    struct sockaddr_in server_addr;   
    struct hostent *host;   
    
        /* 使用hostname查询host 名字 */  
    if(argc!=2)   
    {   
        fprintf(stderr,"Usage:%s hostname \a\n",argv[0]);   
        exit(1);   
    }   
    
    if((host=gethostbyname(argv[1]))==NULL)   
    {   
        fprintf(stderr,"Gethostname error\n");   
        exit(1);   
    }   
    
    /* 客户程序开始建立 sockfd描述符 */   
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:Internet;SOCK_STREAM:TCP  
    {   
        fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));   
        exit(1);   
    }   
    
    /* 客户程序填充服务端的资料 */   
    bzero(&server_addr,sizeof(server_addr)); // 初始化,置0  
    server_addr.sin_family=AF_INET;          // IPV4  
    server_addr.sin_port=htons(portnumber);  // (将本机器上的short数据转化为网络上的short数据)端口号  
    server_addr.sin_addr=*((struct in_addr *)host->h_addr); // IP地址  
        
    /* 客户程序发起连接请求 */   
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)   
    {   
        fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));   
        exit(1);   
    }   
        
    pthread_t id;  
    
    pthread_create(&id,NULL,(void *)read_msg,(int *)&sockfd);  
    /* 连接成功了 */   
    while(1)  
    {  
        printf("Please input char:\n");  
        
        /* 发送数据 */  
        scanf("%s",buffer);   
        write(sockfd,buffer,strlen(buffer));   
    }  
    
    /* 结束通讯 */   
    close(sockfd);   
    exit(0);   
}   