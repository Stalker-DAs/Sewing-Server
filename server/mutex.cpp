#include "mutex.h"
#include "log.h"

namespace server{

Semaphore::Semaphore(uint32_t count){
    if (sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    if (sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify(){
    if (sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}

Mutex::Mutex(){
    pthread_mutex_init(&m_mutex, nullptr);
}

Mutex::~Mutex(){
    pthread_mutex_destroy(&m_mutex);
}

void Mutex::lock(){
    pthread_mutex_lock(&m_mutex);
}

void Mutex::unlock(){
    pthread_mutex_unlock(&m_mutex);
}

RWMutex::RWMutex(){
    pthread_rwlock_init(&m_lock, nullptr);
}

RWMutex::~RWMutex(){
    pthread_rwlock_destroy(&m_lock);
}

void RWMutex::rdlock(){
    pthread_rwlock_rdlock(&m_lock);
}

void RWMutex::wrlock(){
    pthread_rwlock_wrlock(&m_lock);
}


void RWMutex::unlock(){
    pthread_rwlock_unlock(&m_lock);
}

Spinlock::Spinlock(){
    pthread_spin_init(&m_mutex, 0);
}

Spinlock::~Spinlock(){
    pthread_spin_destroy(&m_mutex);
}

void Spinlock::lock(){
    pthread_spin_lock(&m_mutex);
}

void Spinlock::unlock(){
    pthread_spin_unlock(&m_mutex);
}

CASLock::CASLock(){
    m_mutex.clear();
}

void CASLock::lock(){
    while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
}

void CASLock::unlock(){
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    
}


}

