#pragma once

#include <iostream>
#include <string>
#include <stdint.h>
#include <sstream>
#include <memory>
#include <list>
#include <tuple>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include "util.h"
#include "singleton.h"
#include "mutex.h"

// 两种方式，一种是流式，一种是类似C的输出方式
#define LOG_LEVEL(logger, level)                                                                       \
    if (logger->getLevel() <= level)                                                                   \
        server::LogEventWrap(logger, server::LogEvent::ptr(new server::LogEvent(logger->getName(), __FILE__, __LINE__, 0,      \
                                                        server::GetThreadId(), server::GetFiberId(), time(0), level))).getStream() 

#define LOG_DEBUG(logger) LOG_LEVEL(logger, server::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, server::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, server::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, server::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, server::LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                       \
    if (logger->getLevel() <= level)                                                                   \
        server::LogEventWrap(logger, server::LogEvent::ptr(new server::LogEvent(logger->getName(), __FILE__, __LINE__, 0,      \
                                                        server::GetThreadId(), server::GetFiberId(), time(0), level))).getEvent()->format(fmt, __VA_ARGS__)

#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, server::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, server::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, server::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, server::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, server::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() server::LoggerMgr::GetInstance()->retRootLogger()
#define LOG_GET_LOGGER(name) server::LoggerMgr::GetInstance()->getLogger(name)
#define LOG_MAKE_LOGGER(name) server::LoggerMgr::GetInstance()->makeLogger(name)

namespace server{

// 记录log的级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char *ToString(LogLevel::Level level);
    static LogLevel::Level fromString(const std::string &str);
};

// 记录log事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::string logname, const char* file, int32_t line, uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time, LogLevel::Level level);

    const char *getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    LogLevel::Level getLevel() const { return m_level; }

    std::string getLogName() const { return m_logname; }
    std::stringstream& getStream() { return m_ss; }
    std::string getStreamStr() const { return m_ss.str(); }

    void format(const char *fmt, ...);
    void format(const char *fmt, va_list al);

private:
    const char* m_file = nullptr;   //文件名
    int32_t m_line = 0;             //行号
    uint32_t m_elapse = 0;          //启动的毫秒数
    uint32_t m_threadId = 0;        //进程Id
    uint32_t m_fiberId = 0;         //协程Id
    uint64_t m_time;                //时间戳
    std::stringstream ss;           //输出流

    LogLevel::Level m_level;        //当前时间的级别
    std::string m_logname;          //日志器名称
    std::stringstream m_ss;         //输入流
};

// 定义输出的格式，对格式进行解析等操作。
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    // 用于存储不同的执行
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){};
        virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
    };

    LogFormatter(std::string format);
    std::string getFormat() const { return m_format; };
    void setFormatter(std::string format) { m_format = format; };
    std::string format(LogEvent::ptr event);
    

    void init();

private:
    std::vector<FormatItem::ptr> m_formatlist;
    std::string m_format;
};

// 用于定义将语句输出到什么上，比如文件、命令行等。
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    void setFormatter(LogFormatter::ptr val) { m_formatter=val; };
    void setLevel(LogLevel::Level level) { m_level = level; };
    bool FormatterNone() { return m_formatter == nullptr ? true : false; };
    virtual ~LogAppender(){};
    virtual void Log(LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;      //每个appender得有个固定格式。

    Spinlock m_mutex;
};

// 对外的接口，日志器
class Logger
{
public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(std::string name = "root", LogLevel::Level level = LogLevel::DEBUG);
    void Log(LogEvent::ptr event);
    const std::string &getName() const { return m_name; };
    LogLevel::Level getLevel() const { return m_level; };
    LogFormatter::ptr getFormatter() const { return m_formatter; };
    void setLevel(LogLevel::Level level) { m_level = level; };

    void add_Appender(LogAppender::ptr appender);
    void clear_Appender() { m_Appender.clear(); };
    void del_Appender(LogAppender::ptr appender);

    std::string toYamlString();

private:
    std::string m_name;
    LogLevel::Level m_level;

    std::list<LogAppender::ptr> m_Appender;
    LogFormatter::ptr m_formatter;

    Spinlock m_mutex;
};

// 作用是在使用宏的时候声明一个匿名变量，并在使用完之后直接调用析构函数进行输出
class LogEventWrap{
public:
    LogEventWrap(Logger::ptr logger, LogEvent::ptr event);
    ~LogEventWrap();

    std::stringstream& getStream();
    LogEvent::ptr getEvent() const { return m_event; }

private:
    Logger::ptr m_logger;
    LogEvent::ptr m_event;
};


class StdoutLogAppender:public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void Log(LogEvent::ptr event) override;
    std::string toYamlString() override;

private:
};

class FileLogAppender:public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string &filename);

    void Log(LogEvent::ptr event) override;
    std::string toYamlString() override;
private:
    std::string m_filename;
};

// 用于初始化Log
class LogIniter{
public:
    LogIniter();
};

class LogAppenderDefine{
public:
    int type = 0;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string file;
    std::string formatter;

    bool operator==(const LogAppenderDefine& oth) const{
        return type == oth.type && level == oth.level && file == oth.file && formatter == oth.formatter;
    }
};

// 存放从Yaml读取的log内容
class LogDefine
{
public:
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::vector<LogAppenderDefine> appenders;
    std::string formatter;

    bool operator==(const LogDefine& oth) const{
        return name == oth.name && level == oth.level && appenders == oth.appenders && formatter == oth.formatter;
    }
    bool operator<(const LogDefine& oth) const{
        return name < oth.name;
    }
};

class LoggerManager
{
public:
    typedef std::shared_ptr<LoggerManager> ptr;
    LoggerManager();

    Logger::ptr getLogger(const std::string &name);

    Logger::ptr retRootLogger();

    std::string toYamlString();

    Logger::ptr makeLogger(const std::string &name);

private:
    std::map<std::string, Logger::ptr> m_loggers_map;
    Logger::ptr m_root;

    Spinlock m_mutex;
};

typedef server::Singletion<LoggerManager> LoggerMgr;
}
