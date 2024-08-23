#pragma once

#include "log.h"
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace server{

class IPAddress;

class Address
{
public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

    //域名转IP
    static bool Lookup(std::vector<Address::ptr> &result, const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
    static Address::ptr LookupAny(const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);

    //根据网卡获取本机IP子网等信息
    static bool GetInterfaceAddress(std::multimap < std::string, std::pair<Address::ptr, uint32_t>> &result, int family = AF_UNSPEC);
    //static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &ifce, int family = AF_UNSPEC);

    virtual ~Address() {};

    int getFamily() const;

    virtual const sockaddr *getAddr() const = 0;
    virtual sockaddr *getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream &insert(std::ostream &os) const = 0;
    std::string toString();

    bool operator<(const Address &rhs) const;
    bool operator==(const Address &rhs) const;
    bool operator!=(const Address &rhs) const;

private:
};

class IPAddress:public Address{
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char *address, uint16_t port = 0);

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefix) = 0;
    virtual IPAddress::ptr subnetAddress(uint32_t prefix) = 0;

    virtual uint16_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
    
private:
};

class IPv4Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

    IPv4Address(const sockaddr_in &address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;

    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream &os) const override;

    //获取当前IP的广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix) override;
    //获取当前IP的网络地址
    IPAddress::ptr networkAddress(uint32_t prefix) override;
    //获取当前IP的子网掩码
    IPAddress::ptr subnetAddress(uint32_t prefix) override;

    uint16_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address:public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

    IPv6Address();
    IPv6Address(const sockaddr_in6 &address);
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream &os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix) override;
    IPAddress::ptr networkAddress(uint32_t prefix) override;
    IPAddress::ptr subnetAddress(uint32_t prefix) override;

    uint16_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress:public Address{
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const sockaddr_un &address);
    UnixAddress(const std::string &path);

    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream &insert(std::ostream &os) const override;

    void setAddrLen(uint32_t v);

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknowAddress:public Address{
public:
    typedef std::shared_ptr<UnknowAddress> ptr;

    UnknowAddress(int family);
    UnknowAddress(const sockaddr &addr);
    const sockaddr *getAddr() const override;
    sockaddr *getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream &insert(std::ostream &os) const override;
private:
    sockaddr m_addr;
};

std::ostream &operator<< (std::ostream& os, const Address &addr);

}