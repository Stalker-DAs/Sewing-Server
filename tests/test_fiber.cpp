#include "../server/server.h"

server::Logger::ptr g_logger = LOG_ROOT();

void test(){
    LOG_INFO(g_logger) << "run_in_fiber begin";
    server::Fiber::YieldToHoldBack();
    LOG_INFO(g_logger) << "run_in_fiber end";
    server::Fiber::YieldToHoldBack();
}

void fibermain(){
    server::Fiber::GetThis();
    LOG_INFO(g_logger) << "main begin";
    server::Fiber::ptr fiber(new server::Fiber(test));
    fiber->call();
    LOG_INFO(g_logger) << "main after swapIn";
    fiber->call();
}

int main(){
    std::vector<server::Thread::ptr> vec;

    for (size_t i = 0; i < 5; ++i){
        server::Thread::ptr thr(new server::Thread("name: " + std::to_string(i), fibermain));
        vec.push_back(thr);
    }

    for (auto& it: vec){
        it->join();
    }

    return 0;
}