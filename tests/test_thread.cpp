#include "../server/server.h"
#include <vector>

server::Logger::ptr g_logger = LOG_ROOT();

server::Spinlock s_mutex;

int a = 0;

void fun(){
    for (int i = 0; i < 1000000; ++i)
    {
        server::Spinlock::Lock lock(s_mutex);
        ++a;
    }
}

int main(){
    std::vector<server::Thread::ptr> vec;

    for (size_t i = 0; i < 5; ++i){
        server::Thread::ptr thr(new server::Thread("name: " + std::to_string(i), fun));
        vec.push_back(thr);
    }

    for (auto& it: vec){
        it->join();
    }

    std::cout << a;

    return 0;
}