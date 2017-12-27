#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
 
#define BACKLOG 10
#define MAXSIZE 1024
#define CDWRONG "No such file or directory"

int deal_pwd(int clidata_fd);
int deal_cd(int clidata_fd, char *cmd);
int deal_dir(int clidata_fd);
int deal_get(int clidata_fd, int client_fd, char *file_name);
int deal_put(int clidata_fd, char *file_name);
 
int main(int argc, char *argv[]) {
    int sockfd, client_fd;
    int sodata_fd, clidata_fd;
    int servport = atoi(argv[1]);
    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;
    struct sockaddr_in my_addr_d;
    struct sockaddr_in remote_addr_d;
    //创建套接字
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 || (sodata_fd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("socket create failed!");
        exit(1);
    }
 
    //绑定端口地址
    my_addr.sin_family      = AF_INET;
    my_addr.sin_port        = htons(servport);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero),8);

    my_addr_d.sin_family      = AF_INET;
    my_addr_d.sin_port        = htons(servport+1);
    my_addr_d.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr_d.sin_zero),8);

    int opt = 1, opt_d = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    setsockopt(sodata_fd,SOL_SOCKET,SO_REUSEADDR,&opt_d,sizeof(opt_d));

    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) < 0 || bind(sodata_fd, (struct sockaddr*)&my_addr_d, sizeof(struct sockaddr)) < 0) {
        perror("bind error");
        exit(1);
    }
 
    //监听端口
    if (listen(sockfd, BACKLOG) < 0 || listen(sodata_fd, BACKLOG) < 0) {
        perror("listen error");
        exit(1); 
    }
 
    while (1) {
        printf("listening...\n");
        int sin_size   = sizeof(struct sockaddr_in);
        int sin_size_d = sizeof(struct sockaddr_in);
        if ((client_fd = accept(sockfd, (struct sockaddr*)&remote_addr, &sin_size)) < 0 ||
            (clidata_fd = accept(sodata_fd, (struct sockaddr*)&remote_addr_d, &sin_size_d)) < 0) {
            perror("accept error!");
            continue;
        }
        printf("Received a connection from %s\n", inet_ntoa(remote_addr.sin_addr));
 
       //子进程段
        if (!fork()) {
            while(1){
                //接受client发送的请示信息
                char buf[MAXSIZE] = {0};   
                if ((read(client_fd, buf, MAXSIZE)) < 0) {
                    perror("reading stream error!");
                    close(client_fd);
                    continue;
                }
                printf("Received -> %s\n",buf);
                
                //向client发送信息
                if(strcmp(buf, "quit") == 0){
                    printf("1\n");
                    break;
                }else if(strcmp(buf, "pwd") == 0){
                    printf("2\n");
                    if(deal_pwd(clidata_fd) < 0)
                        break;
                }else if(strlen(buf) > 2 && buf[0] == 'c' && buf[1] == 'd'){
                    printf("3\n");
                    deal_cd(clidata_fd, buf);
                }else if(strcmp(buf, "dir") == 0){
                    printf("4\n");
                    deal_dir(clidata_fd);
                }else if(strlen(buf) > 4 && buf[0] == 'g' && buf[1] == 'e' && buf[2] == 't' && buf[3] == ' '){
                    printf("5\n");
                    deal_get(clidata_fd, client_fd, buf + 4);
                }else if(strlen(buf) > 4 && buf[0] == 'p' && buf[1] == 'u' && buf[2] == 't' && buf[3] == ' '){
                    printf("6\n");
                    deal_put(clidata_fd, buf+4);
                }else{

                    printf("7\n");
                }
                
            }
            close(client_fd);
            close(clidata_fd);
            printf("end\n");
            exit(0);
        } 
        close(client_fd);
        close(clidata_fd);
    }
    close(sockfd);
    close(sodata_fd);
    return 0;
}

int deal_pwd(int clidata_fd){
    char file_path_getcwd[MAXSIZE] = {0};
    getcwd(file_path_getcwd, MAXSIZE);
    printf("%s\n", file_path_getcwd);
    if(write(clidata_fd, file_path_getcwd, strlen(file_path_getcwd)+1) < 0){
        return -1;
        perror("send error!");
    }
    return 1;
}

int deal_cd(int clidata_fd, char *cmd){
    if(chdir(cmd + 3) < 0) {
        write(clidata_fd, CDWRONG, strlen(CDWRONG)+1);
        return -1;
    }
    char dir[MAXSIZE] = {0};
    getcwd(dir, MAXSIZE);
    write(clidata_fd, dir, strlen(dir)+1);
    return 1;
}

int deal_put(int clidata_fd, char *file_name){
    char file_size_str[100] = {0};
    read(clidata_fd, file_size_str, 100);
    int file_size = atoi(file_size_str);
    FILE *fp = fopen(file_name, "w");
    if(fp == NULL){
        printf("File:\t%s Can Not Open To Write\n", file_name);
        return -1;
    }
    char buffer[MAXSIZE] = {0};
    int length = 0;
    while((length = read(clidata_fd, buffer, MAXSIZE)) > 0){
        if(fwrite(buffer, sizeof(char), length, fp) < length){
            printf("File: %s Write Failed\n", file_name);
            break;
        }
        bzero(buffer, MAXSIZE);
        file_size -= length;
        if(file_size <= 0)
            break;
    }
    fclose(fp);
    printf("\nReceive File: %s successfully!\n", file_name);
    return 1;
}

int deal_get(int clidata_fd, int client_fd, char *file_name){
    FILE *fp = fopen(file_name, "r");
    if(fp == NULL){
        char *wrongmsg = "No such file";
        write(client_fd, wrongmsg, strlen(wrongmsg)+1);
        return -1;
    }
    write(client_fd, "Start...", strlen("Start...")+1);
    char buffer[MAXSIZE];bzero(buffer, MAXSIZE);
    int file_size = 0;
    int length = 0;
    while((length = fread(buffer, sizeof(char), MAXSIZE, fp)) > 0){
        file_size += length;
        bzero(buffer, MAXSIZE);
    }
    char file_size_str[100] = {0};
    sprintf(file_size_str, "%d", file_size);
    write(clidata_fd, file_size_str, strlen(file_size_str)+1);
    fclose(fp);
    fp = fopen(file_name, "r");
    while((length = fread(buffer, sizeof(char), MAXSIZE, fp)) > 0){
        if(write(clidata_fd, buffer, length) < 0){
            printf("Send File:%s Failed./n", file_name); 
            break; 
        }
        bzero(buffer, MAXSIZE);
    }
    fclose(fp); 
    printf("\nFile: %s transfer successfully!\n", file_name);
    return 1;
}

int deal_dir(int clidata_fd){
    char dir[MAXSIZE*10] = {0};
    DIR *dirp; 
    struct dirent *dp;
    printf("1\n");
    dirp = opendir("."); //打开目录指针
    while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
        strcat(dir, dp->d_name);strcat(dir, "\n");
    } 
    printf("2\n");
    write(clidata_fd, dir, strlen(dir)+1);

    printf("3\n");
    (void) closedir(dirp); //关闭目录
    return 1;
}





