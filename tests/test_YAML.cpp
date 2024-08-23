#include <yaml-cpp/yaml.h>
#include <iostream>

int main(){
    YAML::Node node = YAML::LoadFile("../bin/conf/test.yml");
    for (auto it = node.begin(); it != node.end(); ++it){
        if (it->second.IsMap()){
            for (auto itt = it->second.begin(); itt != it->second.end(); ++itt){
                if (itt->second.IsScalar()){
                    std::cout << itt->second << std::endl;
                }
            }
        }
    }
    return 0;
}
