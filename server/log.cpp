#include "log.h"
#include "config.h"

namespace server{

static LogIniter __log_init;


const char* LogLevel::ToString(LogLevel::Level level){
    switch (level)
    {
    #define XX(name)\
        case name: \
            return #name; \
            break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);

#undef XX
    default:
        return "UNKNOW";
        break;
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::fromString(const std::string &str){
#define XX(level, v)            \
{                               \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }                           \
};
    XX(UNKNOW, UNKNOW);
    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    XX(UNKNOW, unknow);
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

#undef XX
    return LogLevel::UNKNOW;
}

LogEvent::LogEvent(std::string logname, const char* file, int32_t line, uint32_t elapse, 
    uint32_t threadId, uint32_t fiberId, uint64_t time, LogLevel::Level level)
    :m_logname(logname)
    ,m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(threadId)
    ,m_fiberId(fiberId)
    ,m_time(time)
    ,m_level(level)
    {}

Logger::Logger(std::string name, LogLevel::Level level):m_name(name), m_level(level){
    std::string default_format = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n";
    m_formatter.reset(new LogFormatter(default_format));
}

void Logger::Log(LogEvent::ptr event){
    if (event->getLevel() >= m_level){
        for (auto& it:m_Appender){          // 检测如果appender没有formatter使用Logger默认的
            Spinlock::Lock lock(m_mutex);
            if (it->FormatterNone())
            {
                it->setFormatter(m_formatter);
            }
            it->Log(event);
        }
    }
}

std::string Logger::toYamlString() {
    Spinlock::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getFormat();
    }

    for(auto& i : m_Appender) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::add_Appender(LogAppender::ptr appender) {
    Spinlock::Lock lock(m_mutex);
    m_Appender.push_back(appender);
}

void Logger::del_Appender(LogAppender::ptr appender){
    Spinlock::Lock lock(m_mutex);
    for (auto it = m_Appender.begin(); it != m_Appender.end(); ++it){
        if (*it == appender){
            m_Appender.erase(it);
            break;
        }
    }
}

LogFormatter::LogFormatter(std::string format):m_format(format){
    init();
}

std::string LogFormatter::format(LogEvent::ptr event){
    std::stringstream ss;
    for (auto& it:m_formatlist){
        it->format(ss, event);
    }
    return ss.str();
}

void LogEvent::format(const char *fmt, ...){
    // va_list 是C/C++中用于处理可变参数的一种机制
    // va_list指向可变参数列表的指针
    va_list al;
    // 使用va_start初始化al，使其指向可变参数列表的第一个参数
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char *fmt, va_list al){
    char *buf = nullptr;
    // vasprintf用于格式化输出
    int len = vasprintf(&buf, fmt, al);
    if (len != -1){
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream& LogEventWrap::getStream(){
    return m_event->getStream();
}

LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event):m_logger(logger),m_event(event){

}

LogEventWrap::~LogEventWrap(){
    m_logger->Log(m_event);
}

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getStreamStr();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getLogName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string& str):m_str(str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        // tm是一个time.h中的结构体，里面包括日期和时间。time_t是一个表示时间的整形数据
        struct tm tm;
        time_t t = event->getTime();
        // localtime_r用于将时间戳转化为本地时间。localtime的安全版本。
        localtime_r(&t, &tm);
        char buf[64];
        // strftime是用于格式化时间的函数。根据用户提供的格式字符串，将时间转换为一个格式化的字符串表示。
        strftime(buf, sizeof(buf), m_str.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_str;
};

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
    FilenameFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string& str):m_string(str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem{
public:
    TabFormatItem(const std::string& str){}
    void format(std::ostream &os, LogEvent::ptr event) override{
        os << "\t";
    }
};

// "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
void LogFormatter::init(){
    std::vector<std::tuple<std::string, std::string, int>> res;

    std::string str;
    int flag = 0;
    for (int i = 0; i < m_format.size(); ++i)
    {
        if (m_format[i] != '%' && !isalpha(m_format[i]) && m_format[i]!= '{' &&  m_format[i]!= '}'){
            str.push_back(m_format[i]);
            res.push_back(std::make_tuple(str, std::string(), 0));
            str.clear();
            continue;
        }
        else if (m_format[i] == '%'){
            int n = i + 1;
            std::string tmp_str;
            int sub_begin;
            if (flag == 0){
                str.push_back(m_format[n]);
                n++;
                flag = 1;
            }
            if (flag == 1 && m_format[n] == '{'){
                sub_begin = n;
                flag = 2;
                while (m_format[n] != '}'){
                    ++n;
                }
                // 减3用于去除两侧的{}。
                tmp_str = m_format.substr(sub_begin + 1, n - i - 3);
            }

            if (flag == 1){
                res.push_back(std::make_tuple(str, std::string(), 1));
                str.clear();
                i = n - 1;
            }
            else if (flag == 2){
                res.push_back(std::make_tuple(str, tmp_str, 1));
                str.clear();
                i = n;
            }
            flag = 0;
            
        }
    }

    // for(auto& it : res) {
    //     std::cout
    //         << std::get<0>(it)
    //         << " : " << std::get<1>(it)
    //         << " : " << std::get<2>(it)
    //         << std::endl;
    // }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                           \
{                                                                            \
    #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
}
    XX(m, MessageFormatItem),
    XX(p, LevelFormatItem),
    XX(r, ElapseFormatItem),
    XX(c, NameFormatItem),
    XX(t, ThreadIdFormatItem),
    XX(n, NewLineFormatItem),
    XX(d, DateTimeFormatItem),
    XX(f, FilenameFormatItem),
    XX(l, LineFormatItem),
    XX(T, TabFormatItem),
    XX(F, FiberIdFormatItem),

#undef XX
    };

    for (auto& it:res){
        if (std::get<2>(it) == 0){
            m_formatlist.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(it))));
        }
        else{
            auto idx = s_format_items.find(std::get<0>(it));
            if (idx == s_format_items.end()){
                m_formatlist.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(it) + ">>")));
            }
            else{
                m_formatlist.push_back(idx->second(std::get<1>(it)));
            }
        }
    }

}


void StdoutLogAppender::Log(LogEvent::ptr event){
    Spinlock::Lock lock(m_mutex);
    std::cout << m_formatter->format(event);
}

std::string StdoutLogAppender::toYamlString() {
    Spinlock::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getFormat();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


FileLogAppender::FileLogAppender(const std::string &filename){
    m_filename = filename;
}

std::string FileLogAppender::toYamlString() {
    Spinlock::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getFormat();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void FileLogAppender::Log(LogEvent::ptr event){
    Spinlock::Lock lock(m_mutex);
    std::cout << "文件输入输出" << std::endl;
}

LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->add_Appender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers_map["root"] = m_root;
}

Logger::ptr LoggerManager::getLogger(const std::string &name){
    {
        Spinlock::Lock lock(m_mutex);
        auto it = m_loggers_map.find(name);
        if (it == m_loggers_map.end()){
            return m_root;
        }
    }

        
    return makeLogger(name);
}

Logger::ptr LoggerManager::retRootLogger(){
    return m_root;
};

Logger::ptr LoggerManager::makeLogger(const std::string &name){
    Spinlock::Lock lock(m_mutex);
    auto it = m_loggers_map.find(name);
    if (it != m_loggers_map.end()){
        return it->second;
    }

    Logger::ptr newLogger(new Logger(name));
    m_loggers_map[name] = newLogger;
    return newLogger;
}

template<>
class LexicalCast<std::string, LogDefine>{
public:
    LogDefine operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        LogDefine ret;
        if (!node["name"].IsDefined()){
            std::cout << "Log config error: log config name is null!!" << std::endl;
            throw std::logic_error("log config name is null");
        }
        else{
            ret.name = node["name"].as<std::string>();
        }
        
        if(node["level"].IsDefined()){
            ret.level = LogLevel::fromString(node["level"].as<std::string>());
        }
        if(node["formatter"].IsDefined()){
            ret.formatter = node["formatter"].as<std::string>();
        }
        if (node["appenders"].IsDefined()){
            for (size_t i = 0; i < node["appenders"].size(); ++i)
            {
                auto tmp = node["appenders"][i];
                if (!tmp["type"].IsDefined()){
                    std::cout << "Log config error: appenders type is null!!" << std::endl;
                    throw std::logic_error("appenders type is null");
                }

                LogAppenderDefine lad;
                std::string type = tmp["type"].as<std::string>();
                if (type == "FileLogAppender"){
                    lad.type = 1;
                    if (tmp["file"].IsDefined())
                    {
                        lad.file = tmp["file"].as<std::string>();
                    }

                    if (tmp["formatter"].IsDefined()){
                        lad.formatter = tmp["formatter"].as<std::string>();
                    }

                    if (tmp["level"].IsDefined()){
                        lad.level = LogLevel::fromString(tmp["level"].as<std::string>());
                    }
                }
                else if (type == "StdoutLogAppender"){
                    lad.type = 2;
                    if (tmp["formatter"].IsDefined()){
                        lad.formatter = tmp["formatter"].as<std::string>();
                    }

                    if (tmp["level"].IsDefined()){
                        lad.level = LogLevel::fromString(tmp["level"].as<std::string>());
                    }

                }
                else{
                    continue;
                }
                ret.appenders.push_back(lad);
            }
        }

        return ret;
    }
};

template<>
class LexicalCast<LogDefine, std::string>{
public:
    std::string operator()(const LogDefine& num){
        YAML::Node node;
        node["name"] = num.name;
        if (num.level != LogLevel::UNKNOW){
            node["level"] = LogLevel::ToString(num.level);
        }
        if (!num.formatter.empty()){
            node["formatter"] = num.formatter;
        }

        for (auto& it:num.appenders){
            YAML::Node temp;
            if (it.type == 1){
                temp["type"] = "FileLogAppender";
                if (it.file!=""){
                    temp["file"] = it.file;
                }
            }
            if (it.type == 2){
                temp["type"] = "StdoutLogAppender";
            }

            if (!it.formatter.empty()){
                temp["formatter"] = it.formatter;
            }
            node["appenders"].push_back(temp);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

LogIniter::LogIniter(){
    server::Config::ptr config(new server::Config());
    auto log_defines = config->AddData("logs", std::set<LogDefine>(), "");
    log_defines->addListen(0xF1E231, [](const std::set<LogDefine> &old_val,
                                        const std::set<LogDefine> &new_val)
    { 
        LOG_INFO(LOG_ROOT()) << "log file setting has changed!"; 

        for (auto& x: new_val){
            auto it = old_val.find(x);
            server::Logger::ptr logger;
            if (it == old_val.end()){
                logger = LOG_MAKE_LOGGER(x.name);
            }
            else{
                if (!(x == *it)){
                    logger = LOG_MAKE_LOGGER(x.name);
                }
                else{
                    continue;
                }
            }
            if (x.level != LogLevel::UNKNOW){
                logger->setLevel(x.level);
            }

            if (!x.formatter.empty()){
                logger->getFormatter()->setFormatter(x.formatter);
            }

            if (!x.appenders.empty()){
                logger->clear_Appender();
                for (auto &a : x.appenders)
                {
                    LogAppender::ptr la;
                    if (a.type == 1)
                    {
                        la.reset(new FileLogAppender(a.file));
                    }
                    else if (a.type == 2){
                        la.reset(new StdoutLogAppender);
                    }

                    if (a.level != LogLevel::UNKNOW){
                        la->setLevel(a.level);
                    }
                    else{
                        la->setLevel(logger->getLevel());
                    }

                    if (!a.formatter.empty()){

                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        la->setFormatter(fmt);
                    }
                    else{
                        LogFormatter::ptr fmt(new LogFormatter(logger->getFormatter()->getFormat()));
                        la->setFormatter(fmt);
                    }

                    logger->add_Appender(la);
                }
            }

            for (auto &i : old_val){
                auto it = new_val.find(i);
                if (it == new_val.end()){
                    //删除logger
                    auto logger = LOG_GET_LOGGER(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clear_Appender();
                }
            } 

        }
        

    });
}

std::string LoggerManager::toYamlString() {
    Spinlock::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers_map) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


}