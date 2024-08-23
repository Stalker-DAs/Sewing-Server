#include <iostream>
#include "../server/log.h"
#include "../server/util.h"

int main()
{
    // server::Logger::ptr logger(new server::Logger("MyLogger", server::LogLevel::DEBUG));
    // server::StdoutLogAppender::ptr append(new server::StdoutLogAppender());
    // server::LogFormatter::ptr format(new server::LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // logger->add_Appender(append);

    auto logger = server::LoggerMgr::GetInstance()->getLogger("");

    LOG_DEBUG(logger) << "Add";
    LOG_FMT_DEBUG(logger, "dddd %s", "ss");

    return 0;
}