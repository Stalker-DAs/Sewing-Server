#pragma once

#include <memory>
#include "bytearray.h"

namespace server{
class Stream{
public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {};
    virtual int read(void *buf, size_t length) = 0;
    virtual int read(ByteArray::ptr ba, size_t length) = 0;
    virtual int readFixSize(void *buf, size_t length);
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    virtual int write(const void *buf, size_t length) = 0;
    virtual int write(ByteArray::ptr ba, size_t length) = 0;
    virtual int writeFixSize(const void *buf, size_t length);
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;

private:
};
}