#pragma once

#include "Sock.hpp"
#include "Log.hpp"

#define WEBROOT "./wwwroot"
#define HOMEPAGE "index.html"

class Request{
  public:
    Request(){}

    ~Request(){}

    void SetMethod(std::string s) {
      _method = move(s);
    }

    std::string& GetMethod() {
      return _method;
    }

    void SetUri(std::string s) {
      _uri = move(s);
    }

    std::string& GetUri() {
      return _uri;
    }

    void SetVersion(std::string s) {
      _version = move(s);
    }

    void Print() {
        std::cout << _method << " " << _uri << " " << _version << std::endl;
        for (auto& e : _headMap) {
          std::cout << e.first << ": " << e.second << std::endl;
        }
        std::cout << std::endl;

        std::cout << _body << std::endl;
    }

    void InsertToKVMap(std::string&& key, std::string&& val) {
        _headMap.insert(make_pair(std::forward<std::string>(key), std::forward<std::string>(val))); 
    }

    void PrintMap() {
      for (auto& e : _headMap) {
        std::cout << e.first << ": " << e.second << std::endl;
      }
    }

    void SetBody(std::string&& s) {
      _body = std::forward<std::string>(s); 
    }

    int GetContentLength() {
        return _contentLength;
    }
    void SetContentLength(int len) {
        _contentLength = len;    
    }

    void SetPath(std::string&& s) {
        _queryPath = std::forward<std::string>(s);
    }

    std::string& GetQueryPath() {
      return _queryPath;
    }

    void SetQueryParameter(std::string&& s) {
        _queryParameter = std::forward<std::string>(s);
    }

    std::string& GetQueryParameter() {
        return _queryParameter;
    }

    std::string& GetBody() {
        return _body;
    }

    bool IsGet() {
        return !strcasecmp(_method.c_str(), "GET");
    }

    bool IsPost() {
        return !strcasecmp(_method.c_str(), "POST");
    }

    bool IsAddHomePage() {
      if (_queryPath.back() == '/') {
          _queryPath += HOMEPAGE;
          return true;
      } 
      return false;
    }

    void GetQueryPathAndStringFromUri() {
      if (IsGet()) {
          //???uri??????path?????????
          size_t pos = _uri.find('?'); 
          if (pos == std::string::npos) { // ?????????
              _queryPath += _uri;
          }
          else { // ?????????
              _queryPath += std::string(_uri.begin(), _uri.begin() + pos); // ? ??????
              _queryParameter = std::string(_uri.begin() + pos + 1, _uri.end()); // ? ??????
          }
          
      }
      else if (IsPost()) {
          // ???uri????????????
          // ???body????????????
          _queryPath += _uri;

      }
      else {
        assert(false);
      }
      //????????????/?????????????????? index.html
      IsAddHomePage();
    }

    bool PathIsLegal() {
        if (stat(_queryPath.c_str(), &_pathAttribution) == 0) {
          std::cout << "path is legal !" << std::endl;
          return true;
        }
        else {
          
          std::cout << "path is not legal !" << std::endl;
          return false;
        }
    }
        
    bool PathIsDirectory() {
        return S_ISDIR(_pathAttribution.st_mode); 
    }
    void PathAdd(const std::string& s) {
        _queryPath += s;
    }

    bool PathIsBin() {
        auto m = _pathAttribution.st_mode;
        if (m & S_IXUSR || m & S_IXGRP || m & S_IXOTH) {
            return true;
        }
        else {
            return false;
        }
    }

    struct stat GetPathAttribution() {
        return _pathAttribution;
    }

    struct stat Stat(const std::string& s) {
       struct stat st;
       stat(s.c_str(), &st);
       return st;
    }
        
  private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::unordered_map<std::string, std::string> _headMap;
    int  _contentLength = 0;
    std::string _body;
    std::string _queryPath = WEBROOT;
    std::string _queryParameter;
    struct stat _pathAttribution;
};

class Response{
  public:
    Response()
      :_blank("\r\n")
    {}

    std::string CodeToDesc(const std::string code) {
        int num = stoi(code);
        std::string desc;
        switch(num) {
          case 200:
              desc = "OK!";
            break;
          case 404:
              desc = "Not Found!";
            break;
        }
        return desc;
    }

    void MakeStatusLine(const std::string code) {
        _statusLine += "HTTP/1.0";
        _statusLine += " ";
        _statusLine += code;
        _statusLine += " ";
        _statusLine += CodeToDesc(code);
        _statusLine += "\r\n";

    }

    void MakeHeader() {

    }

    std::string& GetStatusLine() {
        return _statusLine;
    }

    std::vector<std::string>& GetHeader() {
        return _header;
    }

  private:
    std::string _statusLine;
    std::vector<std::string> _header;
    std::string _blank;
};

class EndPoint{
  public:
    EndPoint(int sock)
      :_sock(sock)
    {}

    ~EndPoint(){
      if (_sock > 0) {
        close(_sock);
      }
    }

    void AnalysisAndSetRequestLine(std::string s) {
      std::string method, uri, version;

      std::stringstream ss(move(s));
      
      ss >> method >> uri >> version;

      _req.SetMethod(move(method));
      _req.SetUri(move(uri));
      _req.SetVersion(move(version));
        
    }

    void AnalysisAndSetRequestHeader(std::vector<std::string> head) {
      for (size_t i = 0; i < head.size(); i++) {
          std::string key, val; 
          size_t pos = head[i].find(": ");
          key = head[i].substr(0, pos);
          val = head[i].substr(pos + 2);
          if (strcasecmp(key.c_str(), "Content-Length") == 0) {
              _req.SetContentLength(stoi(val));
          }
          _req.InsertToKVMap(move(key), move(val));
      } 
    }
    
    bool IsNeededGetBody() {
        // ?????????????????????GET, POST && Content-Length ?????? || Content-Length = 0 
        std::string& method = _req.GetMethod();
        if (strcasecmp(method.c_str(), "GET") == 0) {
            return false;
        }

        if (strcasecmp(method.c_str(), "POST") == 0 && _req.GetContentLength() == 0) {
            return false; 
        }

        return true;
    }

    void SetRequestBody(std::string&& s) {
        _req.SetBody(std::forward<std::string>(s));
    }

    void PrintRequest() {
       _req.Print();
       std::cout << "??????: " << _req.GetQueryPath() << " ??????: " << _req.GetQueryParameter() << std::endl;
    } 
    // ????????????
    void RecvRequest(){
      //???????????????
      std::string requestLine = Sock::GetOneLine(_sock);  
      AnalysisAndSetRequestLine(move(requestLine));

      //???????????????????????????
      std::vector<std::string> header = Sock::GetRequestHeader(_sock);
      AnalysisAndSetRequestHeader(move(header));
      
      //????????????
      if (IsNeededGetBody()) {
        std::string body = Sock::GetRequestBody(_sock, _req.GetContentLength());
        Log("Notice", body);
        SetRequestBody(move(body));
      }

    }

    // ???????????????????????????
    void HandlerAndMakeResponse() {
        //??????uri???????????????
        // GET??????: uri???????????????????????????
        // POST??????: uri??????????????????????????????
        std::string code;

        _req.GetQueryPathAndStringFromUri();  


        // ????????????
        // version code code-reason
        // Content-Length: 
        // Content-Type:
        // body
        if (!_req.PathIsLegal()) {
           code = "404"; 
           _rsp.MakeStatusLine(code);
           return;
        }
        
        // ??????
        _rsp.MakeStatusLine("200");
        if (_req.PathIsDirectory()) {
            _req.PathAdd("/");
            _req.IsAddHomePage();
        }

        
        // ???????????? cgi

    }

    

    void SendStatusLine() {

        std::string& line = _rsp.GetStatusLine();
        std::cout << "send status line" <<std::endl;
        send(_sock, line.c_str(), line.size(), 0);
    }

    void SendHeader() {
        std::cout << "send header" <<std::endl;
        std::vector<std::string>& head = _rsp.GetHeader();
        for (size_t i = 0; i < head.size(); i++) {
            send(_sock, head[i].c_str(), head[i].size(), 0);  
        }

       // ???????????? 
        std::string s;
        s += "\r\n";
        std::cout << "send blank line" <<std::endl;
        send(_sock, s.c_str(), s.size(), 0);
    }

    void SendBody() {
      if (_req.PathIsBin()) {
         //cgi 
         ExecCgi(); 
      } 
      else {
        //????????????
        //std::cout << "none cgi" << std::endl;
        //std::cout << _req.GetQueryPath().c_str() << std::endl;
        struct stat st = _req.Stat(_req.GetQueryPath());
        int infd = open(_req.GetQueryPath().c_str(), O_RDONLY, 444);
        sendfile(_sock, infd, nullptr, st.st_size);
      }
    }

    // ????????????
    void SendResponse() {
      SendStatusLine();

      SendHeader();

      SendBody();
    }

    void ExecCgi() {
        // ?????? get: uri    pos: body 
        // ????????????
        std::string methodEnv = "METHOD=";
        methodEnv += _req.GetMethod();
        putenv((char*)methodEnv.c_str());


        std::string args;// ??????body??????
        if (_req.IsGet()) {
            std::string argsEnv = "ARG=";//??????????????????
            argsEnv += _req.GetQueryParameter(); 
            putenv((char*)argsEnv.c_str());
        }
        else if (_req.IsPost()) {
          
          std::string contentLength = "Content-Length=";
          contentLength =  std::to_string(_req.GetContentLength());
          putenv((char*)contentLength.c_str());  
          args = _req.GetBody();
        }
        else {
           // other method 
        }

       
        // ??????????????????????????????
        int infd[2] = {0};
        int outfd[2] = {0};
      
        // ??????????????????
        pipe(infd);
        pipe(outfd);

        int id = fork(); 
        if (id == 0) {
            close(infd[1]); 
            close(outfd[0]); 
            dup2(infd[0], 0);
            dup2(outfd[1], 1);
            // ?????????
            execl(_req.GetQueryPath().c_str(), nullptr);    
            exit(0);
        }

        close(infd[0]);
        close(outfd[1]);

        for (size_t i = 0; i < args.size(); i++) {
            char c = args[i];
            write(infd[1], &c, 1);
        }

        int s = 1;
        while (s > 0) {
            char c = 'a';
            s = read(outfd[0], &c, 1);
            if (s > 0) {
                // ????????????
                send(_sock, &c, 1, 0);
            }
        }

        waitpid(id, nullptr, 0);
        close(infd[1]);
        close(outfd[0]);
    }

  private:
    int _sock;
    Request _req;
    Response _rsp;
};

//????????????
class Entry{
  public:
    static void *Routine(void *args){
      pthread_detach(pthread_self());
      int sock = *(int*)args;
      delete (int*)args;

      //char buffer[10240];
      //ssize_t s = recv(sock, buffer, sizeof(buffer), 0);    
      //buffer[s] = 0;
      //std::cout << buffer << std::endl;
      //close(sock);
      
      EndPoint* ep = new EndPoint(sock);

      ep->RecvRequest();

      ep->HandlerAndMakeResponse();
      //ep->PrintRequest();

      ep->SendResponse();

      std::cout << "delete ep" << std::endl;

      delete ep; // ?????????


      return nullptr;

    }


  private:

};
