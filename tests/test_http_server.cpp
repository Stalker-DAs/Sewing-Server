#include "../server/http/http_server.h"
#include "../server/http/servlet.h"

static server::Logger::ptr g_logger = LOG_ROOT();

// void run(){
//     server::http::HttpServer::ptr server(new server::http::HttpServer);
//     server::Address::ptr addr = server::Address::LookupAnyIPAddress("0.0.0.0:8020");
//     while (!server->bind(addr)){
//         sleep(2);
//     }
//     server->start();
// }

void run(){
    server::http::HttpServer::ptr server(new server::http::HttpServer(true));
    server::Address::ptr addr = server::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)){
        sleep(2);
    }
    server::http::ServletManager::ptr(new server::http::ServletManager);
    auto SerManager = server->getServletManager();
    auto servlet = SerManager->addServlet("/server/xx", server::http::Servlet::ptr(new server::http::Servlet("xx")));
    servlet->setGet([](server::http::HttpRequest::ptr req,
                       server::http::HttpResponse::ptr rsp,
                       server::http::HttpSession::ptr session)
                    { rsp->setBody(req->toString()); 
                   return 0; });

    auto glob_servlet = SerManager->addGlobServlet("/server/*", server::http::Servlet::ptr(new server::http::Servlet("xx*")));
    glob_servlet->setGet([](server::http::HttpRequest::ptr req,
                            server::http::HttpResponse::ptr rsp,
                            server::http::HttpSession::ptr session)
                         { rsp->setBody("Glob:\r\n" + req->toString());
        rsp->setHeader("connection", "keep-alive");
        return 0; });

    server->start();
}


int main(){
    server::IOManager iom(2);
    iom.scheduler(run);
    return 0;
}