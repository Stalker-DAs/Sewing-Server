#pragma once

#include "../tcp_server.h"
#include "servlet.h"
#include <memory>

namespace server{
namespace http{
class HttpServer:public TcpServer{
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false, IOManager *worker = IOManager::GetThis(), IOManager *acceptWorker = IOManager::GetThis());

    ServletManager::ptr getServletManager() const { return m_servManager; }
    void setServletManager(ServletManager::ptr v) { m_servManager = v; }

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_isKeepalive;
    ServletManager::ptr m_servManager;
};
}

}