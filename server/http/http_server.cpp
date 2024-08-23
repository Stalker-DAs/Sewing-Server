#include "http_server.h"
#include "http_session.h"
#include "../log.h"

namespace server{
namespace http{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

HttpServer::HttpServer(bool keepalive, IOManager *worker, IOManager *acceptWorker)
:TcpServer(worker, acceptWorker)
,m_isKeepalive(keepalive){
    m_servManager.reset(new ServletManager);
}

void HttpServer::handleClient(Socket::ptr client){
    HttpSession::ptr session(new HttpSession(client));
    do
    {
        auto req = session->recvRequest();
        if (!req){
            LOG_WARN(g_logger) << "recv http request fail, errno="
                                     << errno << " errstr=" 
                                     << strerror(errno) << " client:" << *client;
            //continue;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
        m_servManager->handle(req, rsp, session);
        
        // LOG_INFO(g_logger) << std::endl << "request:" << std::endl
        //                          << *req;
        // LOG_INFO(g_logger) << std::endl << "response:" << std::endl
        //                          << *rsp;

        session->sendResponse(rsp);

    } while (m_isKeepalive);
    session->close();
}
}

}