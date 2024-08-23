#include "servlet.h"
#include <fnmatch.h>

namespace server{
namespace http{
int32_t doDefault(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session){
    static const std::string& DEF_BODY = "<html><head><title>405 Method Not Allowed"
        "</title></head><body><center><h1>405 Method Not Allowed</h1></center>"
        "<hr><center>sewing server/1.0.0</center></body></html>";
    
    response->setStatus(server::http::HttpStatus::METHOD_NOT_ALLOWED);
    response->setHeader("Server", "server/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(DEF_BODY);

    return 0;
}

Servlet::Servlet(const std::string &name)
:m_name(name)
,m_doGet(doDefault)
,m_doPost(doDefault)
,m_doPut(doDefault)
,m_doDelete(doDefault)
,m_default(doDefault){

}

int32_t Servlet::handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session)
{
    http::HttpMethod method = request->getMethod();
    switch (method)
    {
    case HttpMethod::GET:
        return m_doGet(request, response, session);
        break;
    case HttpMethod::POST:
        return m_doPost(request, response, session);
        break;
    case HttpMethod::PUT:
        return m_doPut(request, response, session);
        break;
    case HttpMethod::DELETE:
        return m_doDelete(request, response, session);
        break;
    default:
        return m_default(request, response, session);
        break;
    }

    return -1;
}

NotFoundServlet::NotFoundServlet(const std::string &name):Servlet(name){

}

int32_t NotFoundServlet::handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session){
    static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>sewing Server/1.0.0</center></body></html>";
    
    response->setStatus(http::HttpStatus::NOT_FOUND);
    response->setHeader("Server", "server/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(RSP_BODY);

    return 0;
}

ServletManager::ServletManager(){
    m_default.reset(new NotFoundServlet("Sewing server/1.0.0"));
}

int32_t ServletManager::handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session)
{
    auto slt = getMatchedServlet(request->getPath());
    if (slt){
        slt->handle(request, response, session);
    }
    return 0;
}

Servlet::ptr ServletManager::addServlet(const std::string &uri, Servlet::ptr slt){
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri] = slt;
    return m_datas[uri];
}

Servlet::ptr ServletManager::addGlobServlet(const std::string &uri, Servlet::ptr slt){
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it){
        if (it->first == uri){
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
    return slt;
}

void ServletManager::delServlet(const std::string &uri){
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletManager::delGlobServlet(const std::string &uri){
    RWMutexType::WriteLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it){
        if (it->first == uri){
            m_globs.erase(it);
            break;
        }
    }
    return;
}

Servlet::ptr ServletManager::getServlet(const std::string &uri){
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletManager::getGlobServlet(const std::string &uri){
    RWMutexType::ReadLock lock(m_mutex);
    for (auto it = m_globs.begin(); it != m_globs.end(); ++it){
        if (!fnmatch(it->first.c_str(), uri.c_str(), 0)){
            return it->second;
        }
    }
    return nullptr;
}

Servlet::ptr ServletManager::getMatchedServlet(const std::string &uri){
    Servlet::ptr ret = getServlet(uri);
    if (ret)
    {
        return ret;
    }
    ret = getGlobServlet(uri);
    if (ret)
    {
        return ret;
    }
    return m_default;
}

}
}