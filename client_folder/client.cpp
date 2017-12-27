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

#define MAXLINE 4096
int dataSocket, clientSocket; // 数据 指令socket 
char recvLine[MAXLINE] , sendLine[MAXLINE]; // 发送、接受的字符串
string recvMsg; // 将收到的信息转为String 从而使用substr函数
struct sockaddr_in serverAddr;  // 指向包含有本机IP地址及端口号等信息的sockaddr类型的指针 
string usr;

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
	// if((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	// {
	// 	cout << "Fail to create data socket" << endl;
	// 	exit(0);
	// }
	int ret;
	if ((ret = connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr))) == -1)
	{
		cout << "Command socket fails to connect server" << endl;
		exit(0);	
	}
	// if(ret = connect(dataSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	// {
	// 	cout << "Data socket fails to connect server" << endl;
	// 	exit(0);
	// }
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

		select(clientSocket + 1, &client_fd_set, NULL, NULL, &tv);
		if(FD_ISSET(STDIN_FILENO, &client_fd_set)) {
			bzero(input_msg, MAXLINE);
			fgets(input_msg, MAXLINE, stdin);
			// input_msg[strlen(input_msg) - 1] = '\0';
			if(write(clientSocket , input_msg , strlen(input_msg)) == -1) {
				perror("发送消息出错!\n");
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

	while(1) 
	{
		cout << "UserCommand: ";
		// 发送command
		fgets(sendLine, MAXLINE, stdin);
		string sendmsg = "";
		for(int i = 0; i < strlen(sendLine); i++) {
			sendmsg += sendLine[i];
		}
		write(clientSocket , sendLine , strlen(sendLine));
		if(sendmsg.substr(0, 4) == "chat") {
			while(1) {
				cout << usr << " :";
				fgets(sendLine, MAXLINE, stdin);
				string chatmsg = "";
				for(int i = 0; i < strlen(sendLine); i++) {
					chatmsg += sendLine[i];
				}
				if(chatmsg.substr(0, 7) == "sendmsg") {
					write(clientSocket , sendLine , strlen(sendLine));
					cout << "send!" << endl;
					cout << chatmsg << endl;
				}
				if(chatmsg.substr(0, 4) == "exit") {
					break;
				}
				int count = 0;
				memset(recvLine , '\0' , sizeof(recvLine));
				while((count = read(clientSocket, recvLine, MAXLINE)) == 0);
				recvMsg = "";
				for(int i = 0;i < count;i++)
				{
					recvMsg += recvLine[i];
				}
				if(recvMsg.substr(0, 7) == "sendmsg") {
					cout << recvMsg.substr(8, string::npos) << endl;
				} else {
					cout << recvMsg << endl;
				}
			}
		} else {
			// 接受服务器返回的数据 并转化为string
			int count = 0;
			memset(recvLine , '\0' , sizeof(recvLine));
			recvMsg = "";
			while ((count = read(clientSocket, recvLine, MAXLINE)) == 0); // 接受数据
			for(int i = 0;i < count;i++)
			{
				recvMsg += recvLine[i];
			}
			//处理对应的指令
			if(recvMsg.substr(0 , 4) == "help")
			{
				cout << recvMsg.substr(5) << endl;
			}
			else if(recvMsg.substr(0 , 4) == "quit")
			{
				cout << recvMsg.substr(5) << endl;
				break;
			}
			else if(recvMsg.substr(0 , 4) == "get ")
			{
				if(recvMsg.substr(4) == "fail")
					cout << "No such file!" << endl;
				else
				{
					int count = 0;
					char buffer[MAXLINE]; // 存文件数据
					string fileName = recvMsg.substr(4);
					cout << "fileName: " << fileName << endl;
					string fileContent = "";
					while ((count = read(dataSocket, buffer, MAXLINE)) == 0);
					for (int i = 0; i < count; i ++)
						fileContent += buffer[i];
					FILE *fout = fopen(fileName.c_str(), "wb"); // 写回文件
					fwrite(fileContent.c_str(), sizeof(char), count, fout); // 写回文件
					fclose(fout);
				}
			}
			else if(recvMsg.substr(0 , 4) == "put ")
			{
				string fileName = recvMsg.substr(4);
				// string response = "";
				// FILE *fin = fopen(fileName.c_str(), "rb");
				// if(fin == NULL)
				// {
				// 	cout << "Local no such file" << endl;
				// 	response = "fail";
				// 	write(dataSocket, response.c_str(), strlen(response.c_str()));
				// }
				// else
				// {
				// 	response = "";
				// 	int fileSize = 0;
				// 	char buf[MAXLINE]; // 缓冲
				// 	while((fileSize = fread(buf, sizeof(char), MAXLINE , fin)) == 0); // 读文件
				// 	response = string(buf);
				// 	write(dataSocket , response.c_str() , strlen(response.c_str()));
				// }
				// fclose(fin);
			}
			else if(recvMsg.substr(0 , 3) == "pwd")
			{
				cout << recvMsg.substr(4) << endl;
			}
			else if(recvMsg.substr(0 , 3) == "dir")
			{
				cout << recvMsg.substr(4) << endl;
			}
			else if(recvMsg.substr(0 , 2) == "cd")
			{
				cout << recvMsg.substr(3) << endl;
			} else if(recvMsg.substr(0, 5) == "login") {
				usr = recvMsg.substr(6, string::npos);
				cout << "Login!" << endl;
			}
			else
			{
				cout << recvMsg << endl;
			}
				
		}
	}
}