#include "../server/server.h"
#include "../server/iomanager.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

server::Logger::ptr g_logger = LOG_ROOT();

int sock = 0;

void test_fiber(){
    LOG_INFO(g_logger) << "test_fiber sock=" << sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.139.128", &addr.sin_addr.s_addr);
    //inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        server::IOManager::GetThis()->addEvent(sock, server::IOManager::READ, [](){
            LOG_INFO(g_logger) << "read callback";
        });
        server::IOManager::GetThis()->addEvent(sock, server::IOManager::WRITE, [](){
            LOG_INFO(g_logger) << "write callback";
            //close(sock);
            server::IOManager::GetThis()->cancelEvent(sock, server::IOManager::READ);
            close(sock);
        });
    } else {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test_iom(){
    server::IOManager iom(3, false);
    iom.scheduler(&test_fiber);
}

server::Timer::ptr s_timer;

void test_timer(){
    server::IOManager iom(2);
    s_timer = iom.addTimer(1000, []()
                              { LOG_INFO(g_logger) << "hello timer";  
                              static int i = 0;
                                if (++i == 5){
                                    //s_timer->cancel();
                                    s_timer->getManager()->reset(s_timer, 2000, true);
                                }
                              
                              }, true);
}

int main(){
    // test_iom();
    test_timer();

    return 0;
}