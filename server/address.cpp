#include "address.h"
#include "endian.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>
#include <string.h>

namespace server{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

template<class T>
static T createMask(uint32_t prefix){
    return (1 << (sizeof(T) * 8 - prefix) - 1);
}

template<class T>
static uint32_t CountByte(T value){
    uint32_t res = 0;
    while (value){
        res += (value & 1);
        value = value >> 1;
    }
    return res;
}

bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host, int family, int type, int protocol){
    //解析host得到域名和端口
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char *service = NULL;

    //将IPv6的[]这种地址转换成string纯数字
    if (!host.empty() && host[0] == '[')
    {
        const char *end = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
        if (end){
            if (*(end + 1) == ':'){
                service = end + 2;
            }
            node = host.substr(1, end - host.c_str() - 1);
        }
    }

    //将IPv4的转换成纯数字
    if (node.empty()){
        service = (const char *)memchr(host.c_str(), ':', host.size());
        if (service){
            if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)){
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if (node.empty()){
        node = host;
    }
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error){
        LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
                                  << family << ", " << type << ") err=" << error << " errstr=" << strerror(errno);
        return false;
    }


    next = results;
    while (next){
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return true;
}

Address::ptr Address::LookupAny(const std::string &host, int family, int type, int protocol){
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)){
        return result[0];
    }
    return nullptr;
}

std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string &host, int family, int type, int protocol){
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)){
        for (auto& i:result){
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v){
                return v;
            }
        }
    }
    return nullptr;
}

Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen){
    if (addr == nullptr){
        return nullptr;
    }

    Address::ptr res;
    switch (addr->sa_family)
    {
    case AF_INET:
        res.reset(new IPv4Address(*(const sockaddr_in*)addr));
        break;
    case AF_INET6:
        res.reset(new IPv6Address(*(const sockaddr_in6*)addr));
        break;
    case AF_UNIX:
        res.reset(new UnixAddress(*(const sockaddr_un*)addr));
        break;
    default:
        res.reset(new UnknowAddress(*addr));
        break;
    }
    return res;
}

// Address
bool Address::operator<(const Address &rhs) const{
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0){
        return true;
    }
    else if(result > 0){
        return false;
    }
    else if (getAddrLen() < rhs.getAddrLen()){
        return true;
    }
    else{
        return false;
    }

    return false;
}

bool Address::GetInterfaceAddress(std::multimap < std::string, std::pair<Address::ptr, uint32_t>> &result, int family){
    struct ifaddrs *next, *results;
    if (getifaddrs(&results) != 0){
        LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs " << " err=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    for (next = results; next; next = next->ifa_next){
        Address::ptr res;
        uint32_t prefix_len = ~0u;
        if (family != AF_UNSPEC && family != next->ifa_addr->sa_family){
            continue;
        }
        switch (next->ifa_addr->sa_family)
        {
        case AF_INET:
            {
                res = Create(next->ifa_addr, sizeof(sockaddr_in));
                uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                prefix_len = CountByte(netmask);
            }
            break;
        case AF_INET6:
            {
            res = Create(next->ifa_addr, sizeof(sockaddr_in6));
            prefix_len = 0;
            in6_addr netmask = ((sockaddr_in6 *)next->ifa_addr)->sin6_addr;
            for (int i = 0; i < 16; ++i){
                prefix_len += CountByte(netmask.s6_addr[i]);
            }
            }
        default:
            break;
        }

        if (res){
            result.insert(std::make_pair(next->ifa_name, std::make_pair(res, prefix_len)));
        }
    }
    freeifaddrs(results);
    return true;
}

bool Address::operator==(const Address &rhs) const{
    return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const{
    return !(*this == rhs);
}

int Address::getFamily() const{
    return getAddr()->sa_family;
}

std::string Address::toString(){
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

// 域名转ip
IPAddress::ptr IPAddress::Create(const char *address, uint16_t port){
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));

    // AF_UNSPEC表示同时接受IPv4和IPv6的地址。AI_NUMERICHOST表示只解析IP地址不尝试以域名来解析
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try{
        IPAddress::ptr res = std::dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if (res){
            res->setPort(port);
        }
        freeaddrinfo(results);
        return res;
    }catch(...){
        freeaddrinfo(results);
        return nullptr;
    }
    return nullptr;
}

// IPv4
IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port){
    IPv4Address::ptr rt(new IPv4Address);
    //网络上希望是大端字节序
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if (result <= 0){
        LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                                  << port << ") rt = " << result << " errno=" << errno
                                  << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in &address){
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr *IPv4Address::getAddr() const{
    return (sockaddr *)&m_addr;
}

sockaddr *IPv4Address::getAddr(){
    return (sockaddr *)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const{
    return sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream &os) const{
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);

    return os;
}
//后面全1
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix){
    if (prefix > 32){
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= byteswapOnLittleEndian(createMask<uint32_t>(prefix));

    return IPv4Address::ptr(new IPv4Address(baddr));
}
//前面是自己的ip地址
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix){
    if (prefix > 32){
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= ~byteswapOnLittleEndian(createMask<uint32_t>(prefix));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
//前面全1
IPAddress::ptr IPv4Address::subnetAddress(uint32_t prefix){
    if (prefix > 32){
        return nullptr;
    }

    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(createMask<uint32_t>(prefix));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint16_t IPv4Address::getPort() const{
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v){
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

// IPv6
IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port){
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if (result <= 0){
        LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                                  << port << ") rt = " << result << " errno=" << errno
                                  << " errstr=" << strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_port = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 &address){
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr *IPv6Address::getAddr() const{
    return (sockaddr *)&m_addr;
}

sockaddr *IPv6Address::getAddr(){
    return (sockaddr *)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const{
    return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream &os) const{
    os << "[";
    uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
    bool use_zeros = false;
    for (size_t i = 0; i < 8; ++i){
        while (addr[i] == 0 && i < 8){
            ++i;
            use_zeros = true;
            if (i == 8)
            {
                os << "::";
                break;
            }
        }
        if (use_zeros){
            os << ":";
            use_zeros = false;
            continue;
        }
        if (i){
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix){
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix / 8] |= createMask<uint8_t>(prefix % 8);
    for (int i = prefix / 8 + 1; i < 16; ++i){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix){
    sockaddr_in6 naddr(m_addr);
    naddr.sin6_addr.s6_addr[prefix / 8] &= createMask<uint8_t>(prefix % 8);

    return IPv6Address::ptr(new IPv6Address(naddr));
}

IPAddress::ptr IPv6Address::subnetAddress(uint32_t prefix){
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix / 8] = ~createMask<uint8_t>(prefix % 8);

    for (uint32_t i = 0;  i < prefix / 8; ++i)
    {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint16_t IPv6Address::getPort() const{
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v){
    m_addr.sin6_port = byteswapOnLittleEndian(v);
}

//Unix
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const sockaddr_un &address){
    m_addr = address;
}

UnixAddress::UnixAddress(const std::string &path){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    //防止抽象 Unix 域套接字
    if (!path.empty() && path[0] == '\0'){
        --m_length;
    }

    if (m_length > sizeof(m_addr.sun_path)){
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);

}

const sockaddr *UnixAddress::getAddr() const{
    return (sockaddr *)&m_addr;
}

sockaddr *UnixAddress::getAddr(){
    return (sockaddr *)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const{
    return m_length;
}

std::ostream & UnixAddress::insert(std::ostream &os) const{
    if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0'){
        return os << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

void UnixAddress::setAddrLen(uint32_t v){
    m_length = v;
}

//Unknow
UnknowAddress::UnknowAddress(int family){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknowAddress::UnknowAddress(const sockaddr &addr){
    m_addr = addr;
}

const sockaddr* UnknowAddress::getAddr() const{
    return &m_addr;
}

sockaddr *UnknowAddress::getAddr(){
    return &m_addr;
}

socklen_t UnknowAddress::getAddrLen() const{
    return sizeof(m_addr);
}

std::ostream & UnknowAddress::insert(std::ostream &os) const{
    os << "[UnknownAddress family =" << m_addr.sa_family << "]";
    return os;
}

std::ostream &operator<< (std::ostream& os, const Address &addr){
    return addr.insert(os);
}
}