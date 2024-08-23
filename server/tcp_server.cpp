#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace server{
static server::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
       server::Config::AddData("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");
// static server::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
//        server::Config::AddData("tcp_server.read_timeout", (uint64_t)(1000), "tcp server read timeout");



static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

TcpServer::TcpServer(IOManager *worker, IOManager *acceptWorker)
:m_worker(worker)
,m_acceptWorker(acceptWorker)
,m_recvTimeout(g_tcp_server_read_timeout->getVal())
,m_name("server/1.0.0")
,m_isStop(true){

}

TcpServer::~TcpServer(){
    for (auto & i : m_socks){
        i->close();
    }
    //清空vector
    m_socks.clear();
}

bool TcpServer::bind(Address::ptr addr){
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr> &addrs, std::vector<Address::ptr> &fails){
    for (auto& addr : addrs){
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock->bind(addr)){
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()){
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    if (!fails.empty()){
        m_socks.clear();
        return false;
    }

    for (auto& i :m_socks){
        LOG_INFO(g_logger) << "server bind success: " << *i;
    }
    return true;

}

void TcpServer::startAccept(Socket::ptr sock){
    while (!m_isStop)
    {
        Socket::ptr client = sock->accept();
        if (client){
            client->setRecvTimeout(m_recvTimeout);
            m_worker->scheduler(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        }
        else{
            LOG_ERROR(g_logger) << "accept errno=" << errno << " errstr="
                                      << strerror(errno);
        }
    }
    
}

bool TcpServer::start(){
    if (!m_isStop){
        return true;
    }
    m_isStop = false;
    for (auto& sock:m_socks){
        m_acceptWorker->scheduler(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop(){
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->scheduler([this, self]() {
        for (auto& sock:m_socks){
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client){
    LOG_INFO(g_logger) << "handleClient: " << *client;
}


}