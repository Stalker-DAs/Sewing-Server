#pragma once

#include <iostream>
#include <memory>
#include <functional>
#include "util.h"
#include "log.h"
#include "noncopyable.h"
#include "mutex.h"


namespace server{

class Thread:Noncopyable{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::string name, std::function<void()> cb);
    ~Thread();

    void join();

    pid_t getId() const { return m_id; }
    std::string getName() const { return m_name; }

    // 对static的t_thread_name设置名字(一般首字母大写的作为静态函数)
    static Thread *GetThis();
    static void SetName(const std::string &name);
    static std::string GetName();

    static void *run(void *arg);

private:
    //pid是进程标识符
    //pthread是线程标识符
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::string m_name;
    std::function<void()> m_cb;

    Semaphore::ptr m_semaphor = std::make_shared<Semaphore>(0);
};
}