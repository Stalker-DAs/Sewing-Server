#pragma once

#include <iostream>
#include <zconf.h>
#include <sys/syscall.h>

namespace server{

pid_t GetThreadId();

uint32_t GetFiberId();

uint64_t GetCurrentMS();

uint64_t GetCurrentUS();

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp){
        free(vp);
    }
};
}