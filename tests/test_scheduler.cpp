#include "../server/server.h"

server::Logger::ptr g_logger = LOG_ROOT();

void test(){
    static int s_count = 20;
    LOG_INFO(g_logger) << "Info! ThreadId:" << server::GetThreadId() << " FiberId:" << server::GetFiberId() << std::endl;

    //sleep(2);
    for (int i = 0; i < 100000000; ++i){
            for (int j = 0; j < 10; ++j){
    };
    };
    if (--s_count >= 0)
        //server::Scheduler::GetThis()->scheduler(&test, server::GetThreadId());
        server::Scheduler::GetThis()->scheduler(&test);
}

int main(){
    server::Scheduler sc(4, false, "xxx");
    sc.scheduler(test);
    sc.scheduler(test);
    sc.scheduler(test);
    sc.scheduler(test);
    sc.start();
    // sc.scheduler(test);
    // sc.scheduler(test);
    // sc.scheduler(test);
    sc.stop();
    return 0;
}