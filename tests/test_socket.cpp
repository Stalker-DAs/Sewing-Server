#include "../server/socket.h"
#include "../server/server.h"
#include "../server/iomanager.h"
#include "../server/address.h"

static server::Logger::ptr g_logger = LOG_ROOT();

void test_socket(){
    server::IPAddress::ptr addr = server::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr){
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    else{
        LOG_ERROR(g_logger) << "get address error!!";
        return;
    }

    server::Socket::ptr sock = server::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)){
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    }
    else{
        LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt < 0){
        LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if (rt <= 0){
        LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    LOG_INFO(g_logger) << buffs;
}

int main(){
    server::IOManager iom;
    iom.scheduler(&test_socket);
    return 0;
}
