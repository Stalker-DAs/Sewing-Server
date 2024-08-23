#include "../server/http/http_connection.h"
#include "../server/log.h"
#include "../server/iomanager.h"

static server::Logger::ptr g_logger = LOG_ROOT();

void run(){
    server::Address::ptr addr = server::Address::LookupAnyIPAddress("www.sylar.top:80");
    // server::Address::ptr addr = server::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(!addr) {
        LOG_INFO(g_logger) << "get addr error";
        return;
    }

    server::Socket::ptr sock = server::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    server::http::HttpConnection::ptr conn(new server::http::HttpConnection(sock));
    server::http::HttpRequest::ptr req(new server::http::HttpRequest);
    // req->setPath("/");
    // req->setHeader("host", "www.baidu.com");
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");

    LOG_INFO(g_logger) << "req:" << std::endl
        << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if(!rsp) {
        LOG_INFO(g_logger) << "recv response error";
        return;
    }
    LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    LOG_INFO(g_logger) << "=========================================";

    auto r = server::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    // auto r = server::http::HttpConnection::DoGet("http://www.baidu.com/", 300);
    LOG_INFO(g_logger) << "result=" << r->result
                             << " error=" << r->error
                             << " rsp=" << (r->response ? r->response->toString() : "");
}

int main(){
    server::IOManager iom(2);
    iom.scheduler(run);
    return 0;
}