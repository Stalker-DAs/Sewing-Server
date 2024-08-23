#include "thread.h"

namespace server{

static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

Thread::Thread(std::string name, std::function<void()> cb) : m_name(name), m_cb(cb)
{
    if (name.empty()){
        m_name = "UNKNOW";
    }
    
    //创建一个新的进程
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt){
        LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt << "name=" << m_name;
        throw std::logic_error("pthread_create thread fail");
    }

    m_semaphor->wait();
}

Thread::~Thread(){
    if (m_thread){
        // 让线程分离，使得不需要显示调用pthread_join回收
        pthread_detach(m_thread);
    }
}

void Thread::join(){
    if(m_thread){
        LOG_INFO(g_logger) << "join thread " + m_name;
        int rt = pthread_join(m_thread, nullptr);
        if (rt){
            LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt << "name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

Thread *Thread::GetThis(){
    return t_thread;
}

void Thread::SetName(const std::string &name){
    t_thread_name = name;
}

std::string Thread::GetName(){
    return t_thread_name;
}

void *Thread::run(void *arg){
    Thread* thread = (Thread *)arg;

    //注意这里需要在创建新线程之后再对t_thread和t_thread_name赋值
    t_thread = thread;
    SetName(thread->getName());
    thread->m_id = GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    //这里交换是为了线程安全
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    t_thread->m_semaphor->notify();
    cb();
    return 0;
}
}