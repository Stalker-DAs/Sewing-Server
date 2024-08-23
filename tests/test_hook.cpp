#include "../server/hook.h"
#include "../server/iomanager.h"
#include "../server/fd_manager.h"
#include "../server/log.h"

#include <iostream>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

server::Logger::ptr g_logger1 = LOG_ROOT();

void test_sleep(){
    server::IOManager iom(1);
    iom.scheduler([]()
                 { 
                    sleep(2);
                    LOG_INFO(g_logger1) << "sleep 2"; 
                 });
    
    iom.scheduler([]()
                 { 
                    sleep(3);
                 LOG_INFO(g_logger1) << "sleep 3"; });

    LOG_INFO(g_logger1) << "test sleep";
}

void test_sock(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "39.156.66.14", &addr.sin_addr.s_addr);

    LOG_INFO(g_logger1) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LOG_INFO(g_logger1) << "connect rt=" << rt << " errno=" << errno;

    if(rt){
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_INFO(g_logger1) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0){
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LOG_INFO(g_logger1) << "recv rt=" << rt << " errno=" << errno;

    if (rt <= 0){
        return;
    }

    buff.resize(rt);
    LOG_INFO(g_logger1) << buff;
}

int main(){
    //test_sleep();

    server::IOManager iom(1, false);
    iom.scheduler(test_sock);
    //test_sock();
    return 0;
}