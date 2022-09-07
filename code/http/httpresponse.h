#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    // 生成HTTP响应
    void MakeResponse(Buffer& buff); 
    // 消除文件在内存的映射 
    void UnmapFile();
    // 获取文件在内存的映射
    char* File();
    // 获取文件大小
    size_t FileLen() const;
    // 添加错误内容
    void ErrorContent(Buffer& buff, std::string message);
    // 获取状态码
    int Code() const { return code_; }

private:
    // 添加状态行
    void AddStateLine_(Buffer &buff);
    // 添加首部字段
    void AddHeader_(Buffer &buff);
    // 打开响应文件，并添加Content-length首部字段 
    void AddContent_(Buffer &buff);
    // 跳转到错误网页
    void ErrorHtml_();
    // 判断文件类型
    std::string GetFileType_();

    // 状态码
    int code_;
    // 是否长连接
    bool isKeepAlive_;
    // url
    std::string path_;
    // 文件根目录
    std::string srcDir_;
    // 文件映射
    char* mmFile_;
    // 文件属性
    struct stat mmFileStat_;

    // 文件类型
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    // 支持的响应状态码和信息
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // 错误网页
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //HTTP_RESPONSE_H