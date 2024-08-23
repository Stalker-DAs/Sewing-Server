#pragma once

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <map>
#include <unordered_map>
#include <list>
#include <unordered_set>
#include <set>
#include <functional>
#include "log.h"
#include <yaml-cpp/yaml.h>
#include "mutex.h"

namespace server{

class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string &name, const std::string &description = "") : m_name(name), m_description(description){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    };
    virtual ~ConfigVarBase(){}

    const std::string getName() const { return m_name; }
    const std::string getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& str) = 0;
    virtual std::string getTypeName() const = 0;

private:
    std::string m_name;
    std::string m_description;
};

//最普通的数据转换 T to V
template<class T, class V>
class LexicalCast{
public:
    V operator()(const T& num){
        return boost::lexical_cast<V>(num);
    }
};

//偏特化的数据转换
template<class T>
class LexicalCast<std::string, std::vector<T>>{
public:
    std::vector<T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string>{
public:
    std::string operator()(const std::vector<T>& num){
        YAML::Node node;
        for (auto& i : num){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::list<T>>{
public:
    std::list<T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::list<T>, std::string>{
public:
    std::string operator()(const std::list<T>& num){
        YAML::Node node;
        for (auto& i : num){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::set<T>>{
public:
    std::set<T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string>{
public:
    std::string operator()(const std::set<T>& num){
        YAML::Node node;
        for (auto& i : num){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_set<T>>{
public:
    std::unordered_set<T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string>{
public:
    std::string operator()(const std::unordered_set<T>& num){
        YAML::Node node;
        for (auto& i : num){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T>
class LexicalCast<std::string, std::map<std::string, T>>{
public:
    std::map<std::string, T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }

        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string>{
public:
    std::string operator()(const std::map<std::string, T>& num){
        YAML::Node node;
        for (auto& i : num){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>{
public:
    std::unordered_map<std::string, T> operator()(const std::string& num){
        YAML::Node node = YAML::Load(num);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }

        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>{
public:
    std::string operator()(const std::unordered_map<std::string, T>& num){
        YAML::Node node;
        for (auto& i : num){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        } 
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};



template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T &old_value, const T &new_value)> change_function;

    ConfigVar(const std::string &name, const T &default_val, const std::string &description = "") : ConfigVarBase(name, description), m_val(default_val){};

    std::string toString() override{
        std::string str;
        try
        {
            //str = boost::lexical_cast<std::string>(m_val);
            RWMutex::ReadLock lock(m_mutex);
            str = ToStr()(m_val);
        }
        catch(std::exception& e){
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception" << e.what()
            << " convert: " << typeid(m_val).name() << " to string";
        }

        return str;
    }

    bool fromString(const std::string& str) override{
        try
        {
            //m_val = boost::lexical_cast<T>(str);
            setVal(FromStr()(str));
            return true;
        }
        catch(std::exception& e){
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception" << e.what()
            << " convert: " << typeid(m_val).name();
        }

        return false;
    }

    std::string getTypeName() const override { return typeid(T).name(); } 

    const T getVal() {
        RWMutex::ReadLock lock(m_mutex);
        return m_val;
    }

    void setVal(const T &v){
        {
            RWMutex::ReadLock lock(m_mutex);
            if (m_val == v){
                return;
            }
            for (auto it = m_fc_map.begin(); it != m_fc_map.end(); ++it){
                it->second(m_val, v);
            }
        }
        RWMutex::WriteLock lock(m_mutex);
        m_val = v;
    }

    void delListen(uint64_t hash_num){
        RWMutex::WriteLock lock(m_mutex);
        m_fc_map.erase(hash_num);
    }

    void addListen(uint64_t hash_num, const change_function fc){
        RWMutex::WriteLock lock(m_mutex);
        m_fc_map[hash_num] = fc;
    }

private:
    T m_val;
    //监听list
    std::unordered_map<uint64_t, change_function> m_fc_map;

    RWMutex m_mutex;
};

class Config{
public:
    typedef std::shared_ptr<Config> ptr;
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        RWMutex::WriteLock lock(GetMutex());
        std::string t_name(name);
        std::transform(t_name.begin(), t_name.end(), t_name.begin(), ::tolower);
        auto it = getDatas().find(t_name);
        if (it != getDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp){
                LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }
            else{
                LOG_ERROR(LOG_ROOT()) << "Lookup name=" << name << " exists but type not " << typeid(T).name() << " real_type=" << it->second->getTypeName();
                return nullptr;
            }
        }
    }
    
    template<class T>
    static typename ConfigVar<T>::ptr AddData(const std::string& name, const T &val, const std::string &description){
        // 全部转成小写防止重复
        RWMutex::WriteLock lock(GetMutex());
        std::string t_name(name);
        std::transform(t_name.begin(), t_name.end(), t_name.begin(), ::tolower);
        auto it = getDatas().find(t_name);
        if (it == getDatas().end()){
            if (t_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos){
                LOG_ERROR(LOG_ROOT()) << "Added Config name invalid" << name;
                throw std::invalid_argument(t_name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(t_name, val, description));
            getDatas()[t_name] = v;
            return v;
        }
        else{
            LOG_INFO(LOG_ROOT()) << "Config::AddData:: Data " << name << " is repeated.";
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static ConfigVarBase::ptr LookupBase(const std::string name);

    static void LoadFromYaml(const YAML::Node &node);

private:
    static ConfigVarMap& getDatas(){
        static ConfigVarMap s_datas;
        return s_datas;
    }

    static RWMutex& GetMutex(){
        static RWMutex s_mutex;
        return s_mutex;
    }
};

}