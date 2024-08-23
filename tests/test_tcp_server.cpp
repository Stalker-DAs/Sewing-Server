#include "../server/tcp_server.h"
#include "../server/iomanager.h"

static server::Logger::ptr g_logger = LOG_ROOT();

void run(){
    auto addr = server::Address::LookupAny("0.0.0.0:8033");
    auto addr2 = server::UnixAddress::ptr(new server::UnixAddress("/tmp/unix_addr"));
    LOG_INFO(g_logger) << *addr << "-" << *addr2;
    std::vector<server::Address::ptr> addrs;
    addrs.push_back(addr);
    addrs.push_back(addr2);

    server::TcpServer::ptr tcp_server(new server::TcpServer);
    std::vector<server::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails)){
        sleep(2);
    }
    tcp_server->start();
}

int main(){
    server::IOManager iom(2);
    iom.scheduler(run);
    return 0;
}