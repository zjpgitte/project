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
          //从uri获得path和参数
          size_t pos = _uri.find('?'); 
          if (pos == std::string::npos) { // 无参数
              _queryPath += _uri;
          }
          else { // 有参数
              _queryPath += std::string(_uri.begin(), _uri.begin() + pos); // ? 之前
              _queryParameter = std::string(_uri.begin() + pos + 1, _uri.end()); // ? 之后
          }
          
      }
      else if (IsPost()) {
          // 从uri获取路径
          // 从body获取参数
          _queryPath += _uri;

      }
      else {
        assert(false);
      }
      //若路径以/结尾需要添加 index.html
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
        // 没有正文情况：GET, POST && Content-Length 不存 || Content-Length = 0 
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
       std::cout << "路径: " << _req.GetQueryPath() << " 参数: " << _req.GetQueryParameter() << std::endl;
    } 
    // 获取请求
    void RecvRequest(){
      //获取请求行
      std::string requestLine = Sock::GetOneLine(_sock);  
      AnalysisAndSetRequestLine(move(requestLine));

      //获取请求报头和空行
      std::vector<std::string> header = Sock::GetRequestHeader(_sock);
      AnalysisAndSetRequestHeader(move(header));
      
      //获取正文
      if (IsNeededGetBody()) {
        std::string body = Sock::GetRequestBody(_sock, _req.GetContentLength());
        Log("Notice", body);
        SetRequestBody(move(body));
      }

    }

    // 分析请求并制作响应
    void HandlerAndMakeResponse() {
        //分析uri获取路径。
        // GET方法: uri有参数或者没有参数
        // POST方法: uri只有路径。参数在正文
        std::string code;

        _req.GetQueryPathAndStringFromUri();  


        // 制作响应
        // version code code-reason
        // Content-Length: 
        // Content-Type:
        // body
        if (!_req.PathIsLegal()) {
           code = "404"; 
           _rsp.MakeStatusLine(code);
           return;
        }
        
        // 合法
        _rsp.MakeStatusLine("200");
        if (_req.PathIsDirectory()) {
            _req.PathAdd("/");
            _req.IsAddHomePage();
        }

        
        // 静态网页 cgi

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

       // 发送空行 
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
        //静态网页
        //std::cout << "none cgi" << std::endl;
        //std::cout << _req.GetQueryPath().c_str() << std::endl;
        struct stat st = _req.Stat(_req.GetQueryPath());
        int infd = open(_req.GetQueryPath().c_str(), O_RDONLY, 444);
        sendfile(_sock, infd, nullptr, st.st_size);
      }
    }

    // 发送响应
    void SendResponse() {
      SendStatusLine();

      SendHeader();

      SendBody();
    }

    void ExecCgi() {
        // 参数 get: uri    pos: body 
        // 拿到参数
        std::string methodEnv = "METHOD=";
        methodEnv += _req.GetMethod();
        putenv((char*)methodEnv.c_str());


        std::string args;// 通过body传参
        if (_req.IsGet()) {
            std::string argsEnv = "ARG=";//环境变量传参
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

       
        // 相对于被调用进程来说
        int infd[2] = {0};
        int outfd[2] = {0};
      
        // 创建两个管道
        pipe(infd);
        pipe(outfd);

        int id = fork(); 
        if (id == 0) {
            close(infd[1]); 
            close(outfd[0]); 
            dup2(infd[0], 0);
            dup2(outfd[1], 1);
            // 子进程
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
                // 读到数据
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

//线程进入
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

      delete ep; // 短链接


      return nullptr;

    }


  private:

};
