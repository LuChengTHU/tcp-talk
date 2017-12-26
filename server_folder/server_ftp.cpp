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
#include <thread> 
using namespace std;

#define DEFAULT_PORT 9000
#define MAXLINE 4096
#define BACKLOG 20

string path; // 当前路径

struct sockaddr_in serverAddr[2]; // 指向包含有本机IP地址及端口号等信息的sockaddr类型的指针 

void *process(int id)
{
    int dataSocket , commandSocket , serverSocket; // 数据 指令socket 

    //int id = (int)(*((int*)args));
    cout << id << endl;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cout << "Fail to create server socket" << endl;
        return NULL;
    }

    memset(&serverAddr[id] , 0 , sizeof(sockaddr_in));
    serverAddr[id].sin_family = AF_INET; //IPV4
    serverAddr[id].sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地
    serverAddr[id].sin_port = htons(DEFAULT_PORT + id + 10); // 端口号 Host to Network Short

    if (bind(serverSocket, (struct sockaddr*)&serverAddr[id], sizeof(struct sockaddr)) == -1) 
    {
        cout << "Fail to bind " << id << endl;
        return NULL;
    }
    
    if (listen(serverSocket, BACKLOG) == -1) 
    {
        cout << "Fail to listen" << endl;
        return NULL;
    }
    if((commandSocket = accept(serverSocket, (struct sockaddr*)NULL, NULL)) == -1)
    {
        cout << "Fail to accept command socket" << endl;
        return NULL;
    }
    if((dataSocket = accept(serverSocket, (struct sockaddr*)NULL, NULL)) == -1)
    {
        cout << "Fail to accept command socket" << endl;
        return NULL;
    } 
    char pathBuf[MAXLINE];
    cout << "Connect successfully. Waiting for client command" << endl;
    path = string(getcwd(pathBuf, sizeof(pathBuf)));
    cout << "now path is " << path << endl;
    while(1)
    {
        int count = 0;
        char recvLine[50]; // 存储收到的char型数据
        string recvMsg = ""; // 将收到的信息转为String 从而使用substr函数
        while((count = read(commandSocket, recvLine, sizeof(recvLine))) == 0); // 读取服务器返回的字符串
        for(int i = 0;i < count - 1;i++) // count-1!!  g++编译 会多读一个换行！！ 调了好久！！ 
        {
            recvMsg += recvLine[i];
        }
        cout << "receive: " << recvMsg << endl;
        
        if(recvMsg.substr(0 , 4) == "help")
        {
            string response = "help ";
            response += "get [filename]   : get a file from current directory.\n";
            response += "put [filename]   : upload a file to the current directory.\n";
            response += "pwd              : get current directory.\n";
            response += "dir              : get the file list of the current directory.\n";
            response += "cd [folder]/[..] : go into a sub folder or go to the parent folder.\n";
            response += "help             : see the commands.\n";
            response += "quit             : disconnect.\n";
            write(commandSocket, response.c_str(), strlen(response.c_str()));
        }
        else if(recvMsg.substr(0 , 4) == "quit")
        {
            string response = "quit Goodbye!\n";
            write(commandSocket, response.c_str(), strlen(response.c_str()));
            break;
        }
        else if(recvMsg.substr(0 , 4) == "get ")
        {
            string fileName = recvMsg.substr(4); // get文件名
            string filePath = "";
            filePath = path + "/" + fileName; // 获得get文件的路径
            cout << "filePath: " << filePath << endl;
            FILE *fin = fopen(filePath.c_str(), "rb"); // 打开文件
            if(fin == NULL) // 文件不存在
            {
                string response = "get fail";
                write(commandSocket, response.c_str(), strlen(response.c_str()));
                cout << "error! File does not exit" << endl;
            }
            else
            {
                // 返回command socket
                string response = "get ";
                response += fileName;
                cout << response << endl;
                write(commandSocket, response.c_str(), strlen(response.c_str()));
                
                // 返回具体data
                response = "";
                int fileSize = 0;
                char buf[MAXLINE]; // 缓冲
                while((fileSize = fread(buf, sizeof(char), MAXLINE , fin)) == 0); // 读文件
                for(int i = 0;i < fileSize;i++) // count-1!!  g++编译 会多读一个换行！！ 调了好久！！ 
                {
                    response += buf[i];
                }
                cout << "data: " << response << endl;
                write(dataSocket, response.c_str(), strlen(response.c_str()));
                cout << "successfully" << endl;
            }
            fclose(fin);
        }
        else if(recvMsg.substr(0 , 4) == "put ")
        {
            string fileName = recvMsg.substr(4); // get文件名
            string response = recvMsg;
            write(commandSocket, response.c_str(), strlen(response.c_str())); // 返回解析command
            
            int fileSize = 0;
            char buf[MAXLINE]; // 缓冲
            string fileContent = "";
            while((fileSize = read(dataSocket, buf, sizeof(buf))) == 0); // 读传输过来的文件数据
            for(int i = 0;i < fileSize;i++) 
            {
                fileContent += buf[i];
            }
            cout << "fileContent: " << fileContent << endl;
            if(fileContent != "fail")
            {
                string filePath = "";
                filePath = path + "/" + fileName; 
                cout << "filePath: " << filePath << endl;
                
                FILE *fout = fopen(filePath.c_str() , "wb");
                fwrite(fileContent.c_str() , sizeof(char) , strlen(fileContent.c_str()) , fout);
                
                fclose(fout);
            }
            cout << "put file to remote successfully" << endl;
        }
        else if(recvMsg.substr(0 , 3) == "pwd")
        {
            string response = "pwd ";
            response += path;
            write(commandSocket, response.c_str(), strlen(response.c_str()));
        }
        else if(recvMsg.substr(0 , 3) == "dir")
        {
            string response = "dir ";
            DIR *dp;
            struct dirent *fileName;
            dp = opendir(path.c_str());
            if(!dp)
            {
                cout << "open directory error!" << endl;
            }
            else
            {
                while(fileName = readdir(dp))
                {
                    if(strcmp(fileName->d_name, ".") == 0 || strcmp(fileName->d_name, "..") == 0)
                        continue;
                    response += fileName->d_name;
                    response += '\n';
                }
            }
            cout << "dir: " << response << endl;
            write(commandSocket, response.c_str(), strlen(response.c_str()));
            closedir(dp);
        }
        else if(recvMsg.substr(0 , 2) == "cd")
        {
            if(recvMsg.substr(3) == "..") // 返回上一级
            {
                int position = path.rfind("/");
                if(position >= 0)
                    path.erase(position);
                cout << "now path is " << path << endl;
                string response = "cd ";
                response += path;
                write(commandSocket,response.c_str(), strlen(response.c_str()));
            }
            else
            {
                string folderName = recvMsg.substr(3);
                string temp = path;
                if(temp[temp.size() - 1] != '/') // 加/
                    temp += "/";
                if(folderName[folderName.size()] && folderName.size()) // 去除最后一个/
                    folderName.erase(folderName.size() - 1);
                temp += folderName;
                
                cout << "temp: " << temp << endl;
                // 查看加上cd后面的文件夹名后的路径是否存在
                DIR *dir = opendir(temp.c_str());
                if(dir == NULL)
                {
                    cout << "no such folder!";
                    string response = "cd ";
                    response += "no such folder!";
                    write(commandSocket, response.c_str(), strlen(response.c_str()));
                }
                else
                {
                    path = temp; // 存在 更新当前路径
                    string response = "cd ";
                    response += path;
                    cout << "now path is " << response << endl;
                    write(commandSocket, response.c_str(), strlen(response.c_str()));
                    closedir(dir);
                }
            }
        }
        else
        {
            string response = "Command illegal!";
            write(commandSocket, response.c_str(), strlen(response.c_str()));
        }
    }
}

int main(int argc , char** argv)
{
	

    std::thread threads[2];
    int id[2] = {0, 1};
    for (int i = 0; i < 2; i++) {
        threads[i] = std::thread(process, i);
        //pthread_create( &tids[i], NULL, process, id + i );  
	}
    threads[0].join();
    threads[1].join();
    //pthread_exit( NULL );
	return 0;
}
