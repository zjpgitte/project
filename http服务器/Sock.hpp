#pragma once 

#include "Log.hpp"
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>


enum{
  SocketErr = 1,
  SetOptErr,
  BindErr,
  ListenErr,
  AcceptErr
};

#define BACKLOG 5


class Sock{
  public:
    // 创建套接字
    static int Socket(){
      int sock = socket(AF_INET, SOCK_STREAM, 0);
      if (sock < 0) {
        //向日志打印错误信息
        Log("Error", "socket Error!");
        exit(SocketErr);

      }
      return sock;
    }

    static void SetSockOpt(int sock){
      int opt = 1;
      // 设置套接字地址课复用
      int ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
      if (ret < 0) {
        // warning
        Log("warning", "setsockopt error!");
        exit(SetOptErr);
      }
    }

    static void Bind(int sock, int port){
      struct sockaddr_in local;
      local.sin_family = AF_INET;
      local.sin_port = htons(port);
      local.sin_addr.s_addr = htons(INADDR_ANY);
      if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        //log 
        Log("Error", "Bind Error!");
        exit(BindErr);
      }
    }

    static void Listen(int sock){
      if(listen(sock, BACKLOG) < 0){
        Log("Error", "Listen Error!");
        exit(ListenErr);
      } 
    }

    static int Accept(int sock) {
      struct sockaddr_in peer;
      socklen_t len = sizeof(peer);
      int s = accept(sock, (struct sockaddr*)&peer, &len);
      if (s < 0) {
        //log
        Log("Warning", "Accept Error!");
      }


      return s;
    }

    static std::string GetOneLine(int sock) {
      // /n /r /r/n 
      std::string line;
      char c = 'x'; 
      while (c != '\n'){
          recv(sock, &c, 1, 0);
          if (c == '\r') {
            recv(sock, &c, 1, MSG_PEEK);
            if (c == '\n') {
              recv(sock, &c, 1, 0);
            } 
            else {
              c = '\n';
            }
          }
          
          // c = '\n'或正常字符
          line += c;
      }
      line.pop_back(); // 把'\n'弹出来

      return line;
    }

    // 读取报头
    static std::vector<std::string> GetRequestHeader(int sock) {
        std::vector<std::string> head;
        while (1) {
          std::string line = GetOneLine(sock);
          if (line == "") {
            break;
          }

          head.push_back(move(line));
        }

        return head;
    }

    static std::string GetRequestBody(int sock, int contentLength) {
        std::string body; 
        for (int i = 0; i < contentLength; i++) {
           char c; 
           recv(sock, &c, 1, 0);
           body += c;
        } 

        return body;

    }


};
