
#include "Httpserver.hpp"

int main(int argv, char* argc[]){
  HttpServer *hp = new HttpServer(8081);
  hp->InitServer();
  hp->Start();

  return 0;
}
