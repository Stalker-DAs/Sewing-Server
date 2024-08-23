#include "../server/http/http.h"
#include "../server/log.h"

void test_request(){
    server::http::HttpRequest::ptr req(new server::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello world!");

    req->dump(std::cout) << std::endl;
}

void test_response(){
    server::http::HttpResponse::ptr req(new server::http::HttpResponse);
    req->setHeader("X-X", "sylar");
    req->setBody("hello world!");
    req->setStatus((server::http::HttpStatus)400);
    req->setClose(false);

    req->dump(std::cout) << std::endl;
}

int main(){
    test_request();
    test_response();
    return 0;
}