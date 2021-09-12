
#pragma once 

#include <iostream>
#include <pthread.h>
#include "Sock.hpp"
#include "Protocol.hpp"



class HttpServer{
  private:
    int _lsock; // listen sock
    short _port; 
  public:
    HttpServer(short port)
       :_lsock(-1)
        ,_port(port)
    {
    }

    void InitServer(){
      _lsock = Sock::Socket();
      Sock::SetSockOpt(_lsock);
      Sock::Bind(_lsock, _port);
      Sock::Listen(_lsock);
    }


    void Start(){
      
      signal(SIGPIPE, SIG_IGN);
      while(true) {
        int sock = Sock::Accept(_lsock);
        if (sock < 0) {
          continue;
        }

        Log("Notice", "get a new link ...");
        // 创建线程服务该套接字。epoll
        pthread_t tid; 
        int* s = new int(sock);
        if (pthread_create(&tid, nullptr, Entry::Routine, s) != 0) {
            std::cout << "pthread_create error!" << std::endl;
        }
         
      }
    }
};
