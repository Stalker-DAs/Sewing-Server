#include "../server/address.h"
#include "../server/log.h"

#include <map>
#include <string>

server::Logger::ptr g_logger = LOG_ROOT();

void test(){
    std::vector<server::Address::ptr> addrs;

    bool v = server::Address::Lookup(addrs, "www.baidu.com:80", 0, SOCK_STREAM);

    for (size_t i = 0; i < addrs.size(); ++i){
        LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_ipv4(){
    auto addr = server::IPAddress::Create("39.156.66.18");
    if (addr){
        LOG_INFO(g_logger) << addr->toString();
    }
}

void test_InterfaceAddress(){
    std::multimap<std::string, std::pair<server::Address::ptr, uint32_t>> results;
    bool v = server::Address::GetInterfaceAddress(results);

    for (auto &i:results){
        LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
                                 << i.second.second;
    }
}

int main(){
    // test();
    // test_ipv4();
    test_InterfaceAddress();
    return 0;
}