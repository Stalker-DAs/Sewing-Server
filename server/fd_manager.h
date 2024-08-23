#pragma once

#include "hook.h"
#include "singleton.h"
#include "mutex.h"

#include <memory>
#include <vector>

namespace server{

class FdCtx : public std::enable_shared_from_this<FdCtx>{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit() const {return m_isInit;}
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    bool close();

    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() { return m_sysNonblock; }

    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() { return m_userNonblock; }

    void setTimeout(int type, uint64_t time);
    uint64_t getTimeout(int type);

private:
    // 是否初始化
    bool m_isInit;
    // 是否socket
    bool m_isSocket;
    // 是否hook非阻塞
    bool m_sysNonblock;
    // 是否用户主动设置非阻塞
    bool m_userNonblock;
    // 是否关闭
    bool m_isClosed;

    int m_fd;

    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

class FdManager{
public:
    typedef RWMutex RWMutexType;
    FdManager();
    void resizeFds(size_t len);

    FdCtx::ptr add(int fd);
    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_fds;
};

typedef server::Singletion<FdManager> FdMgr;
}