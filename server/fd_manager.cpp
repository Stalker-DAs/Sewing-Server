#include "fd_manager.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


namespace server{

FdCtx::FdCtx(int fd)
:m_isInit(false)
,m_isSocket(false)
,m_sysNonblock(false)
,m_userNonblock(false)
,m_isClosed(false)
,m_fd(fd)
,m_recvTimeout(-1)
,m_sendTimeout(-1)
{
    init();
}

FdCtx::~FdCtx(){

}

bool FdCtx::init(){
    if (m_isInit){
        return m_isInit;
    }
    struct stat fd_stat;
    if (fstat(m_fd, &fd_stat) == -1){
        m_isSocket = false;
        m_isInit = false;
    }
    else{
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if (m_isSocket){
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)){
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    }
    else{
        m_sysNonblock = false;
    }
    m_userNonblock = false;
    m_isClosed = false;

    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t time){
    if (type == SO_RCVTIMEO){
        m_recvTimeout = time;
    }
    else{
        m_sendTimeout = time;
    }
}

uint64_t FdCtx::getTimeout(int type){
    if (type == SO_RCVTIMEO){
        return m_recvTimeout;
    }
    else{
        return m_sendTimeout;
    }
}

FdManager::FdManager(){
    resizeFds(64);
}

void FdManager::resizeFds(size_t len){
    RWMutexType::WriteLock lock(m_mutex);
    m_fds.resize(len);
}

FdCtx::ptr FdManager::add(int fd){
    if (fd > m_fds.size()){
        resizeFds(fd * 1.5);
    }

    RWMutexType::WriteLock lock(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    m_fds[fd] = ctx;
    return ctx;
}

FdCtx::ptr FdManager::get(int fd, bool auto_create){
    if (auto_create){
        add(fd);
    }
    
    RWMutexType::ReadLock lock(m_mutex);
    return m_fds[fd];
}

void FdManager::del(int fd){
    RWMutexType::WriteLock lock(m_mutex);
    if (m_fds.size() < fd){
        return;
    }
    m_fds[fd].reset();
}
}