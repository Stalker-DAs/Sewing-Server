#pragma once

#include <memory>
#include "../socket_stream.h"
#include "http.h"
#include "../uri.h"

namespace server{
namespace http{

struct HttpResult{
    typedef std::shared_ptr<HttpResult> ptr;
    enum class Error
    {
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CREATE_SOCKET_ERROR = 3,
        CONNECT_FAIL = 4,
        SEND_CLOSE_BY_PEER = 5,
        SEND_SOCKET_ERROR = 6,
        TIMEOUT = 7,
    };
    HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
        :result(_result), response(_response), error(_error){};

    //枚举结果，返回指针，错误描述
    int result;
    HttpResponse::ptr response;
    std::string error;
};

class HttpConnection : public SocketStream{
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);

    static HttpResult::ptr DoGet(const std::string &url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");
    
    static HttpResult::ptr DoGet(Uri::ptr url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");
    
    static HttpResult::ptr DoPost(const std::string &url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");
              
    static HttpResult::ptr DoPost(Uri::ptr url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                   , const std::string &url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                   , Uri::ptr url
                                   , uint64_t timeout_ms
                                   , const std::map<std::string, std::string> &headers = {}
                                   , const std::string &body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                                   , Uri::ptr url
                                   , uint64_t timeout_ms);

    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);
private:
};

}
}