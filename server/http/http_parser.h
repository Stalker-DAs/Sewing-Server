#pragma once

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include <memory>

namespace server{
namespace http{

class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    size_t execute(char *data, size_t len);
    bool isFinished();
    bool hasError();

    HttpRequest::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }

    uint64_t getContentLength();

    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodysize();

    const http_parser &getParser() const { return m_parser; }

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    //1000: invalid method
    //1001: invalid version
    //1001: invalid field
    int m_error;
};

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    size_t execute(char *data, size_t len, bool chunk = false);
    bool isFinished();
    bool hasError();

    HttpResponse::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }

    uint64_t getContentLength();

    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodysize();

    const httpclient_parser &getParser() const { return m_parser; }

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    //1000: invalid method
    //1001: invalid version
    //1001: invalid field
    int m_error;

};


}

}