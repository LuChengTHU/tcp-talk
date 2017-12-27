#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
 
#define MAX_STRING_LENGTH 100
#define BUFFER_SIZE 1024

void showhelp();                                                //显示帮助
int  readstring(int sockfd_d);                                  //读取字符串
int  recv_file(int sockfd_d, char *file_name, int file_size);   //接收文件
int  send_file(int sockfd_d, int sockfd, char *cmd);                  //发送文件


 
int main(int argc, char* argv[]) {
    int servport = atoi(argv[2]);
    char *serverip = argv[1];
    int sockfd, sockfd_d;
    struct sockaddr_in serv_addr;
    struct sockaddr_in serv_addr_d;
 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
        (sockfd_d = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error!");
        exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(servport);
    serv_addr.sin_addr.s_addr = inet_addr(serverip);

    bzero(&serv_addr_d, sizeof(serv_addr_d));
    serv_addr_d.sin_family      = AF_INET;
    serv_addr_d.sin_port        = htons(servport+1);
    serv_addr_d.sin_addr.s_addr = inet_addr(serverip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) < 0 ||
        connect(sockfd_d, (struct sockaddr *)&serv_addr_d,sizeof(struct sockaddr)) < 0) {
        perror("connect error!");
        exit(1);
    }
    printf("connecting 100%%\nPlease input your command :\n");

    while(1){
        char cmd1[MAX_STRING_LENGTH];
        scanf("%s", cmd1);
        if(strcmp(cmd1, "quit") == 0){
            write(sockfd, cmd1, strlen(cmd1)+1);
            break;
        }else if(strcmp(cmd1, "?") == 0){
            showhelp();
            continue;
        }else if(strcmp(cmd1, "pwd") == 0){
            write(sockfd, cmd1, strlen(cmd1)+1);
            if(readstring(sockfd_d) < 0) break;
        }else if(strlen(cmd1) == 2 && cmd1[0] == 'c' && cmd1[1] == 'd'){
            char cmd2[MAX_STRING_LENGTH];
            scanf("%s", cmd2);
            cmd1[2] = ' ';
            int i;
            for(i = 0;i < strlen(cmd2)+1;i++) cmd1[i + 3] = cmd2[i];
            write(sockfd, cmd1, strlen(cmd1)+1);
            if(readstring(sockfd_d) < 0) break;
        }else if(strcmp(cmd1, "dir") == 0){
            write(sockfd, cmd1, strlen(cmd1)+1);
            if(readstring(sockfd_d) < 0) break;
        }else if(strcmp(cmd1, "get") == 0){
            char file_name[MAX_STRING_LENGTH] = {0};
            scanf("%s", file_name);
            strcat(cmd1, " ");strcat(cmd1, file_name);
            write(sockfd, cmd1, strlen(cmd1)+1);
            char filemsg[MAX_STRING_LENGTH];bzero(filemsg, MAX_STRING_LENGTH);
            read(sockfd, filemsg, MAX_STRING_LENGTH);
            printf("%s\n", filemsg);
            if(strcmp(filemsg, "Start...") == 0){
                char file_size_str[100] = {0};
                read(sockfd_d, file_size_str, 100);
                int file_size = atoi(file_size_str);
                if(recv_file(sockfd_d, file_name, file_size) < 0) break;
            }
        }else if(strcmp(cmd1, "put") == 0){
            char file_name[MAX_STRING_LENGTH] = {0};
            scanf("%s", file_name);
            strcat(cmd1, " ");strcat(cmd1, file_name);
            send_file(sockfd_d, sockfd, cmd1);
        }else{
            printf("command not found\n");
        }
        

    }    
    close(sockfd);
    return 0;
}

void showhelp(){
    printf("get : get  a file from the server\nput : send a file to   the server\npwd : display the remote path\ndir : display the remote directory\ncd  : change the remote directory\n?   : list the supported commands\nquit: close the connection\n");
}

int readstring(int sockfd_d){
    char buf[MAX_STRING_LENGTH*10] = {0};
    if(read(sockfd_d, buf, MAX_STRING_LENGTH*10) < 0){
        perror("recv error!");
        return -1;
    }
    printf("Received : \n%s\n", buf);
    return 1;
}

int  send_file(int sockfd_d, int sockfd, char *cmd){
    FILE *fp = fopen(cmd+4, "r");
    if(fp == NULL){
        printf("No such file\n");
        return -1;
    }
    write(sockfd, cmd, strlen(cmd)+1);
    char buffer[BUFFER_SIZE] = {0};
    int file_size = 0;
    int length = 0;
    while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0){
        file_size += length;
        bzero(buffer, BUFFER_SIZE);
    }
    char file_size_str[100] = {0};
    sprintf(file_size_str, "%d", file_size);
    write(sockfd_d, file_size_str, strlen(file_size_str)+1);
    fclose(fp);
    fp = fopen(cmd+4, "r");
    while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0){
        if(write(sockfd_d, buffer, length) < 0){
            printf("Send File:%s Failed./n", cmd+4); 
            break; 
        }
        bzero(buffer, BUFFER_SIZE);
    }
    fclose(fp); 
    printf("\nFile: %s transfer successfully!\n", cmd+4);
    return 1;
}

int recv_file(int sockfd_d, char *file_name, int file_size){
    FILE *fp = fopen(file_name, "w");
    if(fp == NULL){
        printf("File:\t%s Can Not Open To Write\n", file_name);
        return -1;
    }
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int length = 0;
    while((length = read(sockfd_d, buffer, BUFFER_SIZE)) > 0){
        
        if(fwrite(buffer, sizeof(char), length, fp) < length){
            printf("File: %s Write Failed\n", file_name);
            break;
        }
        bzero(buffer, BUFFER_SIZE);
        file_size -= length;
        if(file_size <= 0)
            break;
    }
    fclose(fp);
    printf("\nReceive File: %s successfully!\n", file_name);
    
    return 1;
}








