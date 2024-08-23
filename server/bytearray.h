#pragma once

#include <memory>
#include <sys/types.h>
#include <sys/uio.h>
#include <vector>

namespace server{

class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    struct Node
    {
        Node();
        Node(size_t size);
        ~Node();
        char *ptr;
        Node *next;
        size_t size;
    };
    //默认4k
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //Fix 常规数据类型读写
    void writeFint8(int8_t num);
    void writeFuint8(uint8_t num);
    void writeFint16(int16_t num);
    void writeFuint16(uint16_t num);
    void writeFint32(int32_t num);
    void writeFuint32(uint32_t num);
    void writeFint64(int64_t num);
    void writeFuint64(uint64_t num);

    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    //可变常规数据类型读写
    void writeInt32(int32_t num);
    void writeUint32(uint32_t num);
    void writeInt64(int64_t num);
    void writeUint64(uint64_t num);
    void writeFloat(float num);
    void writeDouble(double num);

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();
    float readFloat();
    double readDouble();

    //String类型读写
    void writeStringF16(const std::string &num);
    void writeStringF32(const std::string &num);
    void writeStringF64(const std::string &num);
    void writeStringVint(const std::string &num);
    void writeStringWithoutLength(const std::string &num);

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    size_t getPosition() const { return m_position; }
    
    size_t getSize() const { return m_size; };
    size_t getBaseSize() const { return m_baseSize; }
    size_t getReadSize() const { return m_size - m_position; }

    void setPosition(size_t v);

    void clear();
    void write(const void *buf, size_t size);
    void read(void *buf, size_t size);
    void read(void *buf, size_t size, size_t position);

    bool writeToFile(const std::string &name);
    bool readFromFile(const std::string &name);
    std::string toString();
    std::string toHexString();

    //只获取内容不修改position
    uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

private:
    size_t getCapacity() const { return m_capacity - m_position; }
    void addCapacity(size_t size);
    // 内存块的大小
    size_t m_baseSize;
    // 当前操作位置
    size_t m_position;
    // 当前的总容量
    size_t m_capacity;
    // 当前数据的大小
    size_t m_size;
    // 字节序,默认大端
    int8_t m_endian;
    // 第一个内存块指针
    Node *m_root;
    // 当前操作的内存块指针
    Node *m_cur;
};
}