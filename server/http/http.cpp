#include "http.h"

#include "../log.h"
#include <string.h>
#include <sstream>

namespace server{
namespace http{

static const char *HttpMethodList[] = {
#define XX(num, method, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
    "INVALID_METHOD"
};

static const char *HttpStatusList[] = {
#define XX(num, method, string) #string,
    HTTP_STATUS_MAP(XX)
#undef XX
};

HttpMethod StringtoHttpMethod(const std::string &val){
#define XX(num, method, string)            \
    if (strcmp(#string, val.c_str()) == 0) \
    {                                      \
        return HttpMethod::method;         \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharstoHttpMethod(const char *val){
#define XX(num, method, string)                      \
    if (strncmp(#string, val, strlen(#string)) == 0) \
    {                                                \
        return HttpMethod::method;                   \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

const char* HttpMethodtoString(const HttpMethod &val){
    uint32_t idx = (uint32_t)val;
    if (idx >= sizeof(HttpMethodList) / sizeof(HttpMethodList[0])){
        return "<unknown>";
    }
    return HttpMethodList[idx];
}

const char* HttpStatustoString(const HttpStatus &val){
    switch (val)
    {
    #define XX(num, status, string)     \
        case HttpStatus::status:        \
        return #string;

    HTTP_STATUS_MAP(XX);
    #undef XX
    default:
        return "<unknown>";
    }
}

bool CaseInsenitiveLess::operator()(const std::string &lhs, const std::string &rhs) const{
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
:m_method(HttpMethod::GET)
,m_version(version)
,m_close(close)
,m_path("/")
{

}

void HttpRequest::setHeader(const std::string &key, const std::string &val){
    m_headers[key] = val;
}

void HttpRequest::setParam(const std::string &key, const std::string &val){
    m_parames[key] = val;
}

void HttpRequest::setCookie(const std::string &key, const std::string &val){
    m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string &key){
    m_headers.erase(key);
}

void HttpRequest::delParam(const std::string &key){
    m_parames.erase(key);
}

void HttpRequest::delCookie(const std::string &key){
    m_cookies.erase(key);
}

bool HttpRequest::hasHeader(const std::string &key, std::string* val){
    auto it = m_headers.find(key);
    if (it == m_headers.end()){
        return false;
    }
    if (val){
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string &key, std::string* val){
    auto it = m_parames.find(key);
    if (it == m_parames.end()){
        return false;
    }
    if (val){
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string &key, std::string* val){
    auto it = m_cookies.find(key);
    if (it == m_cookies.end()){
        return false;
    }
    if (val){
        *val = it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream &os){
    //GET /uri HTTP/1.1 \r\n
    //Host: www.baidu.com

    os << HttpMethodtoString(m_method) << " "
       << m_path
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment
       << " HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << "\r\n"
       << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    for (auto& i : m_headers){
        if (strcasecmp(i.first.c_str(), "connection") == 0){
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    if (!m_body.empty()){
        os << "Content-Length: " << m_body.size() << "\r\n\r\n" << m_body;
    }
    else{
        os << "\r\n\r\n";
    }
    return os;
}

std::string HttpRequest::toString(){
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpResponse::HttpResponse(uint8_t version, bool close)
:m_status(HttpStatus::OK)
,m_version(version)
,m_close(close)
{

}

void HttpResponse::setHeader(const std::string &key, const std::string &val){
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string &key){
    m_headers.erase(key);
}

bool HttpResponse::hasHeader(const std::string &key, std::string* val){
    auto it = m_headers.find(key);
    if (it == m_headers.end()){
        return false;
    }
    if (val){
        *val = it->second;
    }
    return true;
}

std::ostream& HttpResponse::dump(std::ostream &os){
    //HTTP/1.0 200 OK
    //Pragma: no-cache
    //Content-Type: text/html
    //Content-Length: 14988
    //Connection: close
    os << "HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << " "
       << (uint32_t)m_status
       << " "
       << (m_reason.empty() ? HttpStatustoString(m_status) : m_reason)
       <<  "\r\n";
    
    for (auto& i : m_headers){
        if (strcasecmp(i.first.c_str(), "connection") == 0){
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

    if (!m_body.empty()){
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    }
    else{
        os << "\r\n\r\n";
    }
    return os;
}

std::string HttpResponse::toString(){
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, HttpRequest req){
    return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, HttpResponse rsp){
    return rsp.dump(os);
}

}


}