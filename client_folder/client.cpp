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
#include <sstream>
using namespace std;

#define MAXLINE 4096
int dataSocket, clientSocket; // 数据 指令socket 
char recvLine[MAXLINE] , sendLine[MAXLINE]; // 发送、接受的字符串
char dataSend[MAXLINE], dataRecv[MAXLINE];
string recvMsg; // 将收到的信息转为String 从而使用substr函数
struct sockaddr_in serverAddr;  // 指向包含有本机IP地址及端口号等信息的sockaddr类型的指针 
string usr;
int id = 0;

int get_file_size(string filepath) {
    FILE *fin = fopen(filepath.c_str(), "rb"); // 打开文件
    if(fin == NULL) // 文件不存在
    {
        cout << "error! File does not exit" << endl;
    }
    string response = "";
    int fileSize = 0;
	int length = 0;
    char filebuf[MAXLINE]; // 缓冲
    while((length = fread(filebuf, sizeof(char), MAXLINE, fin)) > 0){
        fileSize += length;
        bzero(filebuf, MAXLINE);
    }
	fclose(fin);
	return fileSize;
}

int write_file(int dfd, string filepath) {
	cout << filepath << endl;
    FILE *fin = fopen(filepath.c_str(), "rb"); // 打开文件
    if(fin == NULL) // 文件不存在
    {
        cout << "error! File does not exit" << endl;
    }
    string response = "";
    int fileSize = 0;
	int length = 0;
    char filebuf[MAXLINE]; // 缓冲
    // while((length = fread(filebuf, sizeof(char), MAXLINE, fin)) > 0){
    //     fileSize += length;
    //     bzero(filebuf, MAXLINE);
    // }	
	// char file_size_str[100] = {0};
    // sprintf(file_size_str, "%d", fileSize);
	//write(dfd, file_size_str, strlen(file_size_str)+1);
	// fseek(fin, 0, SEEK_SET);

	bzero(filebuf, MAXLINE);
	length = 0;
    while((length = fread(filebuf, sizeof(char), MAXLINE, fin)) > 0){
		cout << length << endl;
        if(write(dfd, filebuf, length) < 0){
            cout << "Send file " << filepath << " Failed" << endl; 
            break; 
        }
        bzero(filebuf, MAXLINE);
    }
    
    cout << "successfully send " << filepath << endl;
	fclose(fin);
    return fileSize;
}

int read_file(int dfd, string filepath, int fileSize) {
	FILE *fout = fopen(filepath.c_str(), "wb");
	if(fout == NULL) {
		cout << "Cannot write " << filepath << endl;
		return -1;
	}
    char filebuf[MAXLINE]; // 缓冲
	bzero(filebuf, MAXLINE);
	int length = 0;

    while((length = read(dfd, filebuf, MAXLINE)) > 0){
        if(fwrite(filebuf, sizeof(char), length, fout) < length){
            cout << "File write wrong " << filepath << endl;
            break;
        }
        bzero(filebuf, MAXLINE);
        fileSize -= length;
        if(fileSize <= 0)
            break;
    }
    fclose(fout);
    cout << "Read file from client successfully" << endl;
	return 0;
}

int main(int argc , char** argv)
{	
	if(argc != 3) // 判断输入是否合法
	{
		cout << "usage: ./client <ipaddress> port" << endl;
		exit(0);
	}
	memset(&serverAddr , 0 , sizeof(sockaddr_in));
	serverAddr.sin_family = AF_INET; //IPV4
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地
	serverAddr.sin_port = htons(atoi(argv[2])); // 端口号 Host to Network Short
	if(inet_aton(argv[1], &serverAddr.sin_addr) == 0) //判断IP输入是否正确
    {
        cout << "Sever IP address failed" << endl;
		exit(0);
	}
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		cout << "Fail to create command socket" << endl;
		exit(0);
	}
	if((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout << "Fail to create data socket" << endl;
		exit(0);
	}
	int ret;
	if ((ret = connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr))) == -1)
	{
		cout << "Command socket fails to connect server" << endl;
		exit(0);	
	}
	if(ret = connect(dataSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		cout << "Data socket fails to connect server" << endl;
		exit(0);
	}
	cout << "send msg to server" << endl;

    char recv_msg[MAXLINE];
    char input_msg[MAXLINE]; 

	fd_set client_fd_set;
	struct timeval tv;
	while(1) {
		tv.tv_sec = 20;
		tv.tv_usec = 0;
		FD_ZERO(&client_fd_set);
		FD_SET(STDIN_FILENO, &client_fd_set);
		FD_SET(clientSocket, &client_fd_set);

		int ms = max(STDIN_FILENO, clientSocket) + 1;
		select(ms, &client_fd_set, NULL, NULL, &tv);
		if(FD_ISSET(STDIN_FILENO, &client_fd_set)) {
			bzero(input_msg, MAXLINE);
			fgets(input_msg, MAXLINE, stdin);
			string sendmsg = "";
			for(int i = 0; i < strlen(input_msg) - 1; i++) {
				sendmsg += input_msg[i];
			}
			// input_msg[strlen(input_msg) - 1] = '\0';
			if(sendmsg.substr(0, 8) == "sendfile") {
				string filepath = sendmsg.substr(9, string::npos);
				int size = get_file_size(filepath);
				stringstream ss;
				ss << size;
				sendmsg += " " + ss.str();
				cout << "SEND: " << sendmsg << endl;
				if(write(clientSocket , sendmsg.c_str() , strlen(sendmsg.c_str())) == -1) {
					perror("发送消息出错!\n");
				}
				write_file(dataSocket, filepath);
			} else {
				if(write(clientSocket , input_msg , strlen(input_msg)) == -1) {
					perror("发送消息出错!\n");
				}
			}
		}
		if(FD_ISSET(clientSocket, &client_fd_set)) {
			bzero(recv_msg, MAXLINE);
			long byte_num = read(clientSocket, recv_msg, MAXLINE);
			if(byte_num > 0) {
				if(byte_num > MAXLINE) {
					byte_num = MAXLINE;
				}
				//recv_msg[byte_num] = '\0';
				recvMsg = "";
				for(int i = 0;i < byte_num - 1;i++)
				{
					recvMsg += recv_msg[i];
				}
				cout << ">>" + recvMsg << endl;
				if(recvMsg.substr(0,4)=="quit") {
					return 0;
				} else if(recvMsg.substr(0, 3) == "get") {
					//string filepath = recvMsg.substr(4, string::npos);
					//string filename = filepath.substr(filepath.rfind("/") + 1, string::npos);
					string size = recvMsg.substr(4, string::npos);
					stringstream ss2;
					ss2 << size;
					int file_size;
					ss2 >> file_size;
					stringstream ss;
					ss << id;
					string filename = ss.str();
					id++;
                	// char file_size_str[100] = {0};
                	// read(dataSocket, file_size_str, 100);
                	// int file_size = atoi(file_size_str);
					// cout << file_size << endl;
                	if(read_file(dataSocket, "./Client/" + filename, file_size) < 0) {
						break;
						cout << "Read " << filename << " from server, wrong!" << endl;
					}
				}
			} else if(byte_num < 0) {
				printf("接受消息出错!\n");
			} else {
				printf("服务器端退出!\n");
				exit(0);
			}
		}
	}
    return 0;
}