#include "../server/tcp_server.h"
#include "../server/log.h"
#include "../server/bytearray.h"

static server::Logger::ptr g_logger = LOG_ROOT();

class EchoServer : public server::TcpServer{
public:
    EchoServer(int type);
    void handleClient(server::Socket::ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type):m_type(type){

}

void EchoServer::handleClient(server::Socket::ptr client){
    LOG_INFO(g_logger) << "handleClient" << *client;
    server::ByteArray::ptr ba(new server::ByteArray);
    while (true){
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if (rt == 0){
            LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }
        else if (rt < 0){
            LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        //设置size
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if (m_type == 1)
        { // text
            std::cout << ba->toString();
        }
        else{
            std::cout << ba->toHexString();
        }
        std::cout.flush();
    }
}

int type = 0;

void run(){
    EchoServer::ptr es(new EchoServer(type));
    auto addr = server::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }
    es->start();
}

int main(){
    type = 1;
    server::IOManager iom(2);
    iom.scheduler(run);
    return 0;
}