#pragma once

#include <memory>

#include "noncopyable.h"
#include "address.h"
#include "fd_manager.h"
#include "iomanager.h"

namespace server{

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable{
public:
    typedef std::shared_ptr<Socket> ptr;

    enum Type
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };

    enum Family
    {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX, 
    };

    static Socket::ptr CreateTCP(server::Address::ptr address);
    static Socket::ptr CreateUDP(server::Address::ptr address);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);

    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    bool getOption(int level, int option, void *result, size_t *len);

    template<class T>
    bool getOption(int level, int option, T &result){
        size_t length = sizeof(T);
        return getOption(level, optind, &result, &length);
    }

    bool setOption(int level, int option, const void *result, size_t len);

    template<class T>
    bool setOption(int level, int option, const T& value){
        return setOption(level, option, &value, sizeof(T));
    }

    Socket::ptr accept();

    bool init(int sock);
    bool bind(const Address::ptr addr);
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    bool listen(int backlog = SOMAXCONN);
    bool close();

    int send(const void *buffer, size_t length, int flags = 0);
    int send(const iovec *buffers, size_t length, int flags = 0);
    int sendTo(const void *buffer, size_t length, const Address::ptr to, int flags = 0);
    int sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags = 0);

    int recv(void *buffer, size_t length, int flags = 0);
    int recv(iovec *buffers, size_t length, int flags = 0);
    int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags = 0);

    //
    Address::ptr getRemoteAddress();
    //获取服务器绑定的地址和端口号
    Address::ptr getLocalAddress();

    int getSocket() const { return m_sock; }
    int getFamily() const { return m_family; };
    int getType() const { return m_type; };
    int getProtocol() const { return m_protocol; };

    bool isConnected() const { return m_isConnected; };
    bool isValid() const { return m_sock != -1; };

    int getError();

    std::ostream &dump(std::ostream &os) const;

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

private:
    void initSock();
    void newSock();

    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;

    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};

std::ostream &operator<<(std::ostream &os, const Socket &socket);
}