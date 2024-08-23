#pragma once

#include <memory>
#include "stream.h"
#include "socket.h"

namespace server{

class SocketStream : public Stream{
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void *buf, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void *buf, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }
    bool isConnected() const { return m_socket && m_socket->isConnected(); }

private:
    Socket::ptr m_socket;
    bool m_owner;
};
}