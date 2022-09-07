#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    // 请求初始化
    void Init();
    // 解析请求（数据包存在buff）
    bool parse(Buffer& buff);

    // 获取请求的基本信息
    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;

    // 获取post的参数的密码
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    // 是否是长连接
    bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    // 解析http请求行
    bool ParseRequestLine_(const std::string& line);
    // 解析http请求头
    void ParseHeader_(const std::string& line);
    // 解析http消息体
    void ParseBody_(const std::string& line);

    // 解析资源路径
    void ParsePath_();
    // 解析post消息体
    void ParsePost_();
    // 解析url编码，获取POST的参数
    void ParseFromUrlencoded_();

    // 用户验证（登陆或注册）
    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

    // 解析状态
    PARSE_STATE state_;
    // 方法 URL 版本 消息体
    std::string method_, path_, version_, body_;
    // 首部字段
    std::unordered_map<std::string, std::string> header_;
    // 存储POST的参数（用户名&密码）
    std::unordered_map<std::string, std::string> post_;  

    // 默认网页
    static const std::unordered_set<std::string> DEFAULT_HTML;  
    // 注册或登陆
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    // 16进制转10进制
    static int ConverHex(char ch);
};


#endif //HTTP_REQUEST_H