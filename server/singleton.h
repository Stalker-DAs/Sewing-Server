#pragma once
#include <memory>

namespace server{

template <class T>
class Singletion{
public:
    static T* GetInstance(){
        static T v;
        return &v;
    }
};

template <class T>
class SingletionPtr{
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}