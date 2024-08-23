#pragma once

namespace server{

class Noncopyable{
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable &) = delete;

    Noncopyable(Noncopyable &&) = delete;

    Noncopyable operator=(const Noncopyable &) = delete;

};
}