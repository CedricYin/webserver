#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>  // perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h>  // readv
#include <vector>   // readv
#include <atomic>
#include <assert.h>
class Buffer {
public:
    // 初始化缓冲区大小为1024
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    // 可写的字节数
    size_t WritableBytes() const; 
    // 可读的字节数 
    size_t ReadableBytes() const ;
    // 已读的字节数
    size_t PrependableBytes() const;  

    // 返回未读数据的首字符的指针
    const char* Peek() const;
    // 确保还能写len个字节，如果不能，再开辟len大小的可写空间
    void EnsureWriteable(size_t len);
    // 又写了len个字节的数据
    void HasWritten(size_t len);
    // 又读了len个字节的数据
    void Retrieve(size_t len);
    // 读到了end
    void RetrieveUntil(const char* end);
    // 读完了所有数据，缓冲区清空
    void RetrieveAll();
    // 读取所有数据，并以string类型返回
    std::string RetrieveAllToStr();
    // 返回可以写的缓冲区的第一个位置
    const char* BeginWriteConst() const;
    // 返回可以写的缓冲区的第一个位置
    char* BeginWrite();

    // Append：往缓冲区写数据
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    // 读取fd的数据到缓冲区
    ssize_t ReadFd(int fd, int* Errno);
    // 将缓冲区的数据写进fd
    ssize_t WriteFd(int fd, int* Errno);

private:
    // 返回缓冲区的首字符的指针
    char* BeginPtr_();
    const char* BeginPtr_() const;
    // 增加len大小的可写空间
    void MakeSpace_(size_t len);  
    // 缓冲区
    std::vector<char> buffer_;
    // 标志已经读到哪里
    std::atomic<std::size_t> readPos_;
    // 标志已经写到哪里 
    std::atomic<std::size_t> writePos_;
};

#endif //BUFFER_H