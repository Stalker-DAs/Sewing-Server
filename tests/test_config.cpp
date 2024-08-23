#include <iostream>
#include "../server/server.h"
#include <yaml-cpp/yaml.h>
#include <vector>

void config_test(){

    server::Config::ptr config(new server::Config());
    auto num_int = config->AddData("system.num_int", (int)8080, "system port");
    auto num_float = config->AddData("system.num_float", (float)10.3, "system port");
    auto num_vec = config->AddData("system.num_vec", std::vector<int>{1,2,3}, "system port");
    auto num_list = config->AddData("system.num_list", std::list<int>{4,5,6}, "system port");
    auto num_set = config->AddData("system.num_set", std::set<int>{7,8,8,9}, "system port");
    auto num_unset = config->AddData("system.num_unset", std::unordered_set<int>{5,6,7,8,9}, "system port");
    auto num_map = config->AddData("system.num_map", std::map<std::string, int>{{"s", 1}}, "system port");
    auto num_unmap = config->AddData("system.num_unmap", std::unordered_map<std::string, int>{{"s", 2}}, "system port");
    LOG_INFO(LOG_ROOT()) << num_int->toString();
    LOG_INFO(LOG_ROOT()) << num_float->toString();
    LOG_INFO(LOG_ROOT()) << num_vec->toString();
    LOG_INFO(LOG_ROOT()) << num_list->toString();
    LOG_INFO(LOG_ROOT()) << num_set->toString();
    LOG_INFO(LOG_ROOT()) << num_unset->toString();
    LOG_INFO(LOG_ROOT()) << num_map->toString();
    LOG_INFO(LOG_ROOT()) << num_unmap->toString();

    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");
    server::Config::LoadFromYaml(node);

    LOG_INFO(LOG_ROOT()) << num_int->toString();
    LOG_INFO(LOG_ROOT()) << num_float->toString();
    LOG_INFO(LOG_ROOT()) << num_vec->toString();
    LOG_INFO(LOG_ROOT()) << num_list->toString();
    LOG_INFO(LOG_ROOT()) << num_set->toString();
    LOG_INFO(LOG_ROOT()) << num_unset->toString();
    LOG_INFO(LOG_ROOT()) << num_map->toString();
    LOG_INFO(LOG_ROOT()) << num_unmap->toString();


    // 监听者模式，监听修改并执行相应listen函数。
    
    // server::Config::ptr config(new server::Config());
    // auto num_int = config->AddData("system.num_int", (int)8080, "system port");
    // LOG_INFO(LOG_ROOT()) << num_int->toString();

    // num_int->addListen(10, [](const int &old_val, const int &new_val)
    //                    { std::cout << old_val << std::endl;
    //                      std::cout << new_val << std::endl; });

    // YAML::Node node = YAML::LoadFile("../bin/conf/test.yml");
    // server::Config::LoadFromYaml(node);

    // LOG_INFO(LOG_ROOT()) << num_int->toString();
}

void Yaml_config_test(){
    static server::Logger::ptr system_log = LOG_MAKE_LOGGER("system");
    LOG_INFO(system_log) << 1111;        
    std::cout << server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node node = YAML::LoadFile("../bin/conf/log.yml");
    server::Config::LoadFromYaml(node);
    std::cout << server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    LOG_DEBUG(system_log) << 2222;


}

int main(){
    //config_test();
    Yaml_config_test();
    return 0;
}
