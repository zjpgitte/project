#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
using namespace std;

int main() {

  char *s = getenv("METHOD");
  string args;
  if (strcasecmp(s, "GET") == 0) {
     args = getenv("ARG"); 
  }
  else if (strcasecmp(s, "POST")) {
     int len = atoi(getenv("Content-Length"));
     for (int i = 0; i < len; i++) {
       char c = 'a';
       read(0, &c, 1);
       args += c;
     }
  }
  

  cout << "result: " << args << endl;

  return 0;
}
