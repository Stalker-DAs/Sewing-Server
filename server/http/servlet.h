#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include "http.h"
#include "http_session.h"
#include "../macro.h"

namespace server{
namespace http{
class Servlet{
public:
    typedef std::shared_ptr<Servlet> ptr;
    typedef std::function<int32_t(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session)> doFunction;
    Servlet(const std::string &name);
    virtual ~Servlet(){};

    virtual int32_t handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session);

    void setGet(doFunction doF) { m_doGet = doF; };
    void setPost(doFunction doF) { m_doPost = doF; };
    void setPut(doFunction doF) { m_doPut = doF; };
    void setDelete(doFunction doF) { m_doDelete = doF; };

    std::string getName() { return m_name; }

private:
    std::string m_name;
    doFunction m_doGet;
    doFunction m_doPost;
    doFunction m_doPut;
    doFunction m_doDelete;
    doFunction m_default;
};

class NotFoundServlet : public Servlet{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet(const std::string &name);
    virtual int32_t handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session) override;

private:
};

class ServletManager{
public:
    typedef std::shared_ptr<ServletManager> ptr;
    typedef std::function<int32_t(http::HttpRequest::ptr request, http::HttpResponse::ptr response, http::HttpSession::ptr session)> callback;
    typedef RWMutex RWMutexType;

    ServletManager();

    int32_t handle(http::HttpRequest::ptr request,
                   http::HttpResponse::ptr response,
                   http::HttpSession::ptr session);

    //精准匹配
    Servlet::ptr addServlet(const std::string &uri, Servlet::ptr slt);
    //模糊匹配
    Servlet::ptr addGlobServlet(const std::string &uri, Servlet::ptr slt); 

    void delServlet(const std::string &uri);
    void delGlobServlet(const std::string &uri);

    Servlet::ptr getServlet(const std::string &uri);
    Servlet::ptr getGlobServlet(const std::string &uri);
    
    Servlet::ptr getMatchedServlet(const std::string &uri);

private:
    RWMutexType m_mutex;
    // uri -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    // 默认servlet，所有路径都没匹配到时使用
    Servlet::ptr m_default;
};
}

}