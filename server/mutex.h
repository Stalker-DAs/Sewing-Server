#pragma once

#include <semaphore.h>
#include <memory>
#include <atomic>
#include "noncopyable.h"

namespace server{

class Semaphore: Noncopyable{
public:
    typedef std::shared_ptr<Semaphore> ptr;
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:

    sem_t m_semaphore;
};

//锁的RAII机制
template<class T>
struct ScopedLockImpl
{
public:
    ScopedLockImpl(T &mutex):m_mutex(mutex){
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl(){
       unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock(){
        if (m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private: 
    //注意这里一定要是引用
    T& m_mutex;
    bool m_locked;
};


template<class T>
struct ReadScopedLockImpl
{
public:
    ReadScopedLockImpl(T &mutex):m_mutex(mutex){
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl(){
        unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock(){
        if (m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private: 
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl
{
public:
    WriteScopedLockImpl(T &mutex):m_mutex(mutex){
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl(){
        unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock(){
        if (m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


private: 
    T& m_mutex;
    bool m_locked;
};

class Mutex{
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex();
    ~Mutex();
    void lock();
    void unlock();

private:
    pthread_mutex_t m_mutex;
};

class RWMutex{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex();
    ~RWMutex();
    void rdlock();
    void wrlock();
    void unlock();
    
private:
    pthread_rwlock_t m_lock;
};

class Spinlock{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock();
    ~Spinlock();
    void lock();
    void unlock();
    
private:
    pthread_spinlock_t m_mutex;
};

//CAS原子类型锁
class CASLock{
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock();
    ~CASLock(){};
    void lock();
    void unlock();
    
private:
    volatile std::atomic_flag m_mutex;
};
}
