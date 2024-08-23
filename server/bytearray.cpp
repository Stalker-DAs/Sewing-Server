#include "bytearray.h"
#include "log.h"
#include "endian.h"

#include <string.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace server{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

ByteArray::Node::Node()
:ptr(nullptr)
,next(nullptr)
,size(0)
{
}

ByteArray::Node::Node(size_t s)
:ptr(new char[s])
,next(nullptr)
,size(s)
{
}

ByteArray::Node::~Node(){
    if (ptr){
        delete[] ptr;
    }
    next = nullptr;
    size = 0;
}

ByteArray::ByteArray(size_t base_size)
:m_baseSize(base_size)
,m_position(0)
,m_capacity(base_size)
,m_size(0)
,m_endian(SERVER_BIG_ENDIAN)
,m_root(new Node(base_size))
,m_cur(m_root){

}

ByteArray::~ByteArray(){
    Node *tmp = m_root;
    while (tmp){
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_root = nullptr;
    m_cur = nullptr;
}

void ByteArray::writeFint8(int8_t num){
    write(&num, sizeof(num));
}

void ByteArray::writeFuint8(uint8_t num){
    write(&num, sizeof(num));
}
//如果是小端需要转换成大端保存
void ByteArray::writeFint16(int16_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

void ByteArray::writeFuint16(uint16_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

void ByteArray::writeFint32(int32_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

void ByteArray::writeFuint32(uint32_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

void ByteArray::writeFint64(int64_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

void ByteArray::writeFuint64(uint64_t num){
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    write(&num, sizeof(num));
}

int8_t ByteArray::readFint8(){
    int8_t num;
    read(&num, sizeof(num));
    return num;
}

uint8_t ByteArray::readFuint8(){
    uint8_t num;
    read(&num, sizeof(num));
    return num;
}

int16_t ByteArray::readFint16(){
    int16_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

uint16_t ByteArray::readFuint16(){
    uint16_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

int32_t ByteArray::readFint32(){
    int32_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

uint32_t ByteArray::readFuint32(){
    uint32_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

int64_t ByteArray::readFint64(){
    int64_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

uint64_t ByteArray::readFuint64(){
    uint64_t num;
    read(&num, sizeof(num));
    if (SERVER_BYTE_ORDER != m_endian){
        num = byteswap(num);
    }
    return num;
}

static uint32_t EncodeZigzag32(const int32_t& num){
    if (num < 0){
        return ((uint32_t)(-num)) * 2 - 1;
    }
    else{
        return num * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& num){
    if (num < 0){
        return ((uint64_t)(-num)) * 2 - 1;
    }
    else{
        return num * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& num){
    //如果num事奇数(num & 1)为1
    return (num >> 1) ^ -(num & 1);
}

static int64_t DecodeZigzag64(const uint64_t& num){
    return (num >> 1) ^ -(num & 1);
}

void ByteArray::writeInt32(int32_t num){
    writeUint32(EncodeZigzag32(num));
}

void ByteArray::writeUint32(uint32_t num){
    uint8_t res[5];
    uint8_t i = 0;
    while (num >= 0x80)
    {
        res[i++] = ((num & 0x7F) | 0x80);
        num >>= 7;
    }
    res[i++] = num;
    write(res, i);
}

void ByteArray::writeInt64(int64_t num){
    writeUint64(EncodeZigzag64(num));
}
    
void ByteArray::writeUint64(uint64_t num){
    uint8_t res[10];
    uint8_t i = 0;
    while (num >= 0x80)
    {
        res[i++] = ((num & 0x7F) | 0x80);
        num >>= 7;
    }
    res[i++] = num;
    write(res, i);
}
    
void ByteArray::writeFloat(float num){
    uint32_t v;
    memcpy(&v, &num, sizeof(num));
    writeFuint32(v);
}

void ByteArray::writeDouble(double num){
    uint64_t v;
    memcpy(&v, &num, sizeof(num));
    writeFuint64(v);
}

int32_t ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32(){
    uint32_t res = 0;
    for (size_t i = 0; i < 32; i+=7){
        uint8_t tmp = readFuint8();
        if (tmp >= 0x80){
            res |= (((uint32_t)(tmp & 0x7F)) << i);
        }
        else{
            res |= ((uint32_t)tmp) << i;
            break;
        }
    }
    return res;
}

int64_t ByteArray::readInt64(){
    return DecodeZigzag64(readUint64());
}
    
uint64_t ByteArray::readUint64(){
    uint64_t res = 0;
    for (size_t i = 0; i < 64; i+=7){
        uint8_t tmp = readFuint8();
        if (tmp >= 0x80){
            res |= (((uint64_t)(tmp & 0x7F)) << i);
        }
        else{
            res |= ((uint64_t)tmp << i);
            break;
        }
    }
    return res;
}
    
float ByteArray::readFloat(){
    uint32_t v = readFuint32();
    float res;
    memcpy(&res, &v, sizeof(v));
    return res;
}

double ByteArray::readDouble(){
    uint64_t v = readFuint64();
    float res;
    memcpy(&res, &v, sizeof(v));
    return res;
}

void ByteArray::writeStringF16(const std::string &num){
    //先写长度再写数据
    writeFuint16(num.size());
    write(num.c_str(), num.size());
}

void ByteArray::writeStringF32(const std::string &num){
    writeFuint32(num.size());
    write(num.c_str(), num.size());
}
    
void ByteArray::writeStringF64(const std::string &num){
    writeFuint64(num.size());
    write(num.c_str(), num.size());
}
    
void ByteArray::writeStringVint(const std::string &num){
    writeUint64(num.size());
    write(num.c_str(), num.size());
}
    
void ByteArray::writeStringWithoutLength(const std::string &num){
    write(num.c_str(), num.size());
}

std::string ByteArray::readStringF16(){
    uint16_t len = readFuint16();
    std::string res;
    res.resize(len);
    read(&res[0], len);
    return res;
}

std::string ByteArray::readStringF32(){
    uint32_t len = readFuint32();
    std::string res;
    res.resize(len);
    read(&res[0], len);
    return res;
}
    
std::string ByteArray::readStringF64(){
    uint64_t len = readFuint64();
    std::string res;
    res.resize(len);
    read(&res[0], len);
    return res;
}
    
std::string ByteArray::readStringVint(){
    uint64_t len = readUint64();
    std::string res;
    res.resize(len);
    read(&res[0], len);
    return res;
}

void ByteArray::clear(){
    m_position = 0;
    m_capacity = m_baseSize;
    m_size = 0;
    Node *tmp = m_root->next;
    while (tmp){
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void *buf, size_t size){
    if (size == 0){
        return;
    }
    addCapacity(size);

    size_t nbias = m_position % m_baseSize;
    size_t ncap = m_cur->size - nbias;
    size_t buf_bias = 0;
    while (size > 0)
    {
        if (size > ncap){
            memcpy(m_cur->ptr + nbias, (const char *)buf + buf_bias, ncap);
            m_position += ncap;
            buf_bias += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            nbias = 0;
        }
        else{
            memcpy(m_cur->ptr + nbias, (const char *)buf + buf_bias, size);
            m_position += size;
            buf_bias += size;
            size = 0;
        }
    }
    if (m_position > m_size){
        m_size = m_position;
    }
}
    
void ByteArray::read(void *buf, size_t size){
    if (size > getReadSize()){
        throw std::out_of_range("not enough len");
    }
    size_t nbias = m_position % m_baseSize;
    size_t ncap = m_cur->size - nbias;
    size_t buf_bias = 0;
    while (size > 0)
    {
        if (size > ncap){
            memcpy((char *)buf + buf_bias, m_cur->ptr + nbias, ncap);
            m_position += ncap;
            buf_bias += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            nbias = 0;
        }
        else{
            memcpy((char *)buf + buf_bias, m_cur->ptr + nbias, size);
            if (m_cur->size == (nbias + size)){
                m_cur = m_cur->next;
            }
            m_position += size;
            buf_bias += size;
            size = 0;
        }
    }
}

//从指定position读取数据
void ByteArray::read(void *buf, size_t size, size_t position){
    if (size > getReadSize()){
        throw std::out_of_range("not enough len");
    }

    size_t nbias = position % m_baseSize;
    size_t ncap = m_cur->size - nbias;
    size_t buf_bias = 0;
    while (size > 0)
    {
        if (size > ncap){
            memcpy((char *)buf + buf_bias, m_cur->ptr + nbias, ncap);
            position += ncap;
            buf_bias += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            nbias = 0;
        }
        else{
            memcpy((char *)buf + buf_bias, m_cur->ptr + nbias, size);
            if (m_cur->size == (nbias + size)){
                m_cur = m_cur->next;
            }
            position += size;
            buf_bias += size;
            size = 0;
        }
    }
}

//重置m_position和cur
void ByteArray::setPosition(size_t v){
    if(v > m_capacity){
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if (m_position > m_size){
        m_size = m_position;
    }
    m_cur = m_root;
    while (v > m_cur->size){
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if (v == m_cur->size){
        m_cur = m_cur->next;
    }
}

void ByteArray::addCapacity(size_t size){
    if (size == 0){
        return;
    }

    if (getCapacity() > size){
        return;
    }

    size_t old_size = getCapacity();
    size = size - old_size;
    size_t add_node_num = (size / m_baseSize) + ((size % m_baseSize) ? 1 : 0);

    Node *tmp = m_root;
    while (tmp->next)
    {
        tmp = tmp->next;
    }
    Node *ret = nullptr;
    while (add_node_num){
        tmp->next = new Node(m_baseSize);
        if (ret == nullptr){
            ret = tmp->next;
        }
        --add_node_num;
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }
    if (old_size == 0){
        m_cur = ret;
    }
}

bool ByteArray::writeToFile(const std::string &name){
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if (!ofs){
        LOG_ERROR(g_logger) << "writeToFile name=" << name
                                  << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node *cur = m_cur;
    while (read_size > 0){
        int diff = pos % m_baseSize;
        uint64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;

        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}

bool ByteArray::readFromFile(const std::string &name){
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if (!ifs){
        LOG_ERROR(g_logger) << "readFromFile name=" << name << " error, errno=" << errno
                                  << " errstr=" << strerror(errno);
        return false;
    }

    char *buff = new char[m_baseSize];
    while (!ifs.eof())
    {
        //从文件读取
        ifs.read(buff, m_baseSize);
        //写入链表
        write(buff, ifs.gcount());
    }

    delete buff;
    return true;
}

std::string ByteArray::toString(){
    std::string ret;
    ret.resize(getReadSize());
    if (ret.empty()){
        return ret;
    }
    read(&ret[0], ret.size(), m_position);

    return ret;
}

std::string ByteArray::toHexString(){
    std::string str = toString();
    std::stringstream ss;

    for (size_t i = 0; i < str.size(); ++i){
        if (i > 0 && i % 32 == 0){
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len) const{
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0){
        return 0;
    }
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node *cur = m_cur;
    while (len > 0){
        if (ncap >= size){
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else{
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const{
    len = len > getReadSize() ? getReadSize() : len;
    if (len == 0){
        return 0;
    }
    uint64_t size = len;

    size_t npos = position % m_baseSize;

    size_t count = position / m_baseSize;
    Node *cur = m_root;
    while (count > 0)
    {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while (len > 0){
        if (ncap >= size){
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else{
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len){
    if (len == 0){
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node *cur = m_cur;
    while (len > 0){
        if (ncap >= len){
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else{
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}