#include <iostream>
#include "config.h"

namespace server{

ConfigVarBase::ptr Config::LookupBase(const std::string name){
    auto it = getDatas().find(name);
    return it == getDatas().end() ? nullptr : it->second;
}

static void ListAllMember(const std::string prefix, const YAML::Node &node, std::list<std::pair<std::string, YAML::Node>>& res){
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos){
        LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    res.push_back(std::make_pair(prefix, node));
    if (node.IsMap()){
        for (auto it = node.begin(); it != node.end(); ++it){
            ListAllMember(prefix.empty() ? it->first.Scalar()
                                         : prefix + "." + it->first.Scalar(),
                          it->second, res);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node &node){
    RWMutex::ReadLock lock(GetMutex());
    std::list<std::pair<std::string, YAML::Node>> all_nodes;
    ListAllMember("", node, all_nodes);

    for (auto& it:all_nodes){
        std::string key = it.first;
        if (key.empty()){
            continue;
        }
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        auto var = Config::LookupBase(key);
        if (var){
            if (it.second.IsScalar()){
                var->fromString(it.second.Scalar());
            }
            else{
                std::stringstream ss;
                ss << it.second;
                var->fromString(ss.str());
            }
        }
    }
}
}