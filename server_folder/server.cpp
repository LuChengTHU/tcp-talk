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
#include <map>
#include <vector>
#include <set>
using namespace std;

#define DEFAULT_PORT 9000
#define MAXLINE 4096
#define BACKLOG 20
#define MAXUSR 200

std::string path; // 当前路径

struct ServerData {
    std::map<string, int> usrId;
    std::map<string, string> usrPasswd;
    std::map<string, bool> usrLive;
    std::map<string, std::map<string, std::vector<string>>> waitRecvMsg; // <recv, <sender, msg>>
    std::map<string, std::set<string>> usrFriend; //<usr, setfriend>
};

ServerData serverData;

int my_write(int fd,void *buffer,int length)
{
    int bytes_left;
    int written_bytes;
    char *ptr;

    ptr=(char*)buffer;
    bytes_left=length;
    while(bytes_left>0)
    {
        written_bytes=write(fd,ptr,bytes_left);
        if(written_bytes<=0)
        {       
            if(errno==EINTR)
                written_bytes=0;
            else             
                return(-1);
        }
        bytes_left-=written_bytes;
        ptr+=written_bytes;     
    }
    return(0);
}

int my_read(int fd,void *buffer,int length)
{
    int bytes_left;
    int bytes_read;
    char *ptr;
    
    bytes_left=length;
    while(bytes_left>0)
    {
        bytes_read=read(fd,ptr,bytes_read);
        if(bytes_read<0)
        {
            if(errno==EINTR)
                bytes_read=0;
            else
                return(-1);
        }
        else if(bytes_read==0)
            break;
        bytes_left-=bytes_read;
        ptr+=bytes_read;
    }
    return(length-bytes_left);
}

void * read_msg(int fd)  
{    
    int count = 0;  
    char buffer[1024];
    string usr = "";
    
    while((count = read(fd,buffer,sizeof(buffer))) > 0)  
    {  
        string recvMsg = "";
        for(int i = 0;i < count - 1;i++) // count-1!!  g++编译 会多读一个换行！！
        {
            recvMsg += buffer[i];
        }
        memset(buffer,0,sizeof(buffer));

        cout << "receive: " << recvMsg << endl;

        if(recvMsg.substr(0, 4) == "quit") {
            string response = "quit Goodbye!\n";
            write(fd, response.c_str(), strlen(response.c_str()));
            if(strlen(usr.c_str()) != 0) {
                serverData.usrLive[usr] = false;
            }
            break;
        } else if(recvMsg.substr(0, 5) == "login") {
            string usrinfo = recvMsg.substr(6, string::npos);
            std::size_t pos = usrinfo.find(" ");
            usr = usrinfo.substr(0, pos);
            string passwd = usrinfo.substr(pos+1, string::npos);
            serverData.usrPasswd[usr] = passwd;
            serverData.usrId[usr] = fd;
            serverData.usrLive[usr] = true;

            string response = "login " + usr + "\n";
            cout << "Login usr : " + usr + ", fd is ";
            cout << fd << endl;
            write(fd, response.c_str(), strlen(response.c_str()));
        } else if(strlen(usr.c_str()) == 0) {
            string res = "Please login first!\n";
            write(fd, res.c_str(), strlen(res.c_str()));
        } else if(recvMsg.substr(0, 4) == "chat") {
            string recvId = recvMsg.substr(5, string::npos);
            int receiver = serverData.usrId[recvId];

            int chatCount = 0;
            char chatBuffer[1024];
            while((chatCount = read(fd,chatBuffer,sizeof(chatBuffer))) > 0)  
            {  
                string chatRecvMsg = "";
                for(int i = 0;i < chatCount - 1;i++)
                {
                    chatRecvMsg += chatBuffer[i];
                }
                cout << "chatmsg: " << chatRecvMsg << endl;
                memset(chatBuffer,0,sizeof(chatBuffer));
                if(chatRecvMsg.substr(0, 7) == "sendmsg") {
                    string send = usr + ": " + chatRecvMsg.substr(8, string::npos) + "\n";
                    if(serverData.usrLive[recvId]) {
                        write(receiver, send.c_str(), strlen(send.c_str()));
                    } else {
                        serverData.waitRecvMsg[recvId][usr].push_back(send);
                    }
                } else if(chatRecvMsg.substr(0, 4) == "exit") {
                    cout << usr << " exit chat" << endl;
                    break;
                }
            }
        } else if(recvMsg.substr(0, 7) == "recvmsg") {
            auto waitmsg = serverData.waitRecvMsg[usr];
            string res;
            if(waitmsg.empty()) {
                res = "You have no unread message.\n";
            } else {
                res = "Messages: \n"
                for(auto sends = waitmsg.begin(); sends != waitmsg.end(); sends++) {
                    string sender = sends->first;
                    auto sendermsg = sends->second;
                    for(auto msg : sendermsg) {
                        res += msg;
                    }
                }
                serverData.waitRecvMsg[usr].clear();
            }
            write(fd, res.c_str(), strlen(res.c_str()));
        } else if(recvMsg.substr(0, 6) == "search") {
            string res = "Users:\n";
            for(auto usrs = serverData.usrId.begin(); usrs != serverData.usrId.end(); usrs++) {
                res += usrs->first + "\n";
            }
            write(fd, res.c_str(), strlen(res.c_str()));
        } else if(recvMsg.substr(0, 3) == "add") {
            string other = recvMsg.substr(4, string::npos);
            string res = "";
            auto it = serverData.usrId.find(other);
            if(it != serverData.usrId.end()) {
                if(serverData.usrFriend[usr].empty()) {
                    serverData.usrFriend[usr].insert(other);
                    serverData.usrFriend[other].insert(usr);
                    res = "Add " + other + " successfully!\n";
                } else if(serverData.usrFriend[usr].find(other) == serverData.usrFriend[usr].end()) {
                    serverData.usrFriend[usr].insert(other);
                    serverData.usrFriend[other].insert(usr);
                    res = "Add " + other + " successfully!\n";
                } else {
                    res = other + " is already your friend!\n";
                }
            } else {
                res = "User " + other + " does not exit!\n";
            }
            write(fd, res.c_str(), strlen(res.c_str()));
        } else if(recvMsg.substr(0, 2) == "ls") {
            string res;
            if(serverData.usrFriend[usr].empty()) {
                res = "You have no friends!\n";
            } else {
                res = "Your friends: \n";
                for(auto it = serverData.usrFriend[usr].begin(); it != serverData.usrFriend[usr].end(); it++) {
                    res += *it + "\n";
                }
            }
            write(fd, res.c_str(), strlen(res.c_str()));
        } else if(recvMsg.substr(0, 7) == "profile") {
            string res = "";
            res += "User: " + usr + "\n";
            res += "Password: " + serverData.usrPasswd[usr] + "\n";
            write(fd, res.c_str(), strlen(res.c_str()));
        }
    }
}

int main(int argc, char *argv[])
{
    int serverSocket, clientSocket;
    struct sockaddr_in server_addr;  ; // 指向包含有本机IP地址及端口号等信息的sockaddr类型的指针 
    std::thread threads[MAXUSR];
    int cnum = 0;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cout << "Fail to create server socket" << endl;
        exit(1);
    }

    memset(&server_addr , 0 , sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET; //IPV4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地
    server_addr.sin_port = htons(DEFAULT_PORT); // 端口号 Host to Network Short

    if (bind(serverSocket, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) 
    {
        cout << "Fail to bind " <<endl;
        exit(1);
    }
    
    cout << "Server begin" << endl;

    if (listen(serverSocket, BACKLOG) == -1) 
    {
        cout << "Fail to listen" << endl;
        exit(1);
    }

    while(1) {
        cout << cnum << endl;
        if((clientSocket = accept(serverSocket, (struct sockaddr*)NULL, NULL)) == -1)
        {
            cout << "Fail to accept command socket" << endl;
            exit(1);
        }
        cout << "Connect successfully. Waiting for client command" << endl;
        // char pathBuf[MAXLINE];
        
        // path = string(getcwd(pathBuf, sizeof(pathBuf)));
        // cout << "now path is " << path << endl;
        threads[cnum] = std::thread(read_msg, clientSocket);
        threads[cnum].detach();
        cnum += 1;
    }
}