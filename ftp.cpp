#include "ftp.h"
#include <mutex>
namespace ftpclient {

namespace {
bool IsMatchedMultiline(const char *buf, size_t count, const char *&parseEnd)
{
    // 匹配ftp多行回复格式
    static const std::regex pattern( R"((^[1-5][0-5][0-9])-.*\r\n(.*\r\n)*(^\1 .*\r\n))");
    auto start = buf;
    auto end   = buf + count;
    std::cmatch  m;
    if (std::regex_search(start, end, m, pattern) && (m.position() == 0)) {
        parseEnd = buf + m.length();
        return true;
    } else {
        parseEnd = nullptr;
        return false;
    }
}

bool IsMatchedOneLine(const char * buf, size_t count, const char *&parseEnd)
{
    // 匹配单行回复
    static const std::regex pattern( R"((^[1-5][0-5][0-9]) (.)*\r\n)");
    auto start = buf;
    auto end   = buf + count;
    std::cmatch  m;
    if (std::regex_search(start, end, m, pattern) && (m.position() == 0)) {
        parseEnd = buf + m.length();
        return true;
    } else {
        parseEnd = nullptr;
        return false;
    }
}

bool IsOneLineFormat(const char * buf, size_t count)
{
    static const std::regex pattern( R"([1-5][0-5][0-9] )");
    auto start = buf;
    auto end   = buf + 4;
    return std::regex_match(start, end, pattern);
}

bool IsMultiLineFormat(const char * buf, size_t count)
{
    static const std::regex pattern( R"([1-5][0-5][0-9]-)");
    auto start = buf;
    auto end   = buf + 4;
    return std::regex_match(start, end, pattern);
}

} // anonymouse namespace 

FTPClientConfig *FTPClientConfig::GetInstance()
{
    static FTPGlobalConfig *instance = nullptr;
    if (!instance) {
        std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new FTPClientConfig();
        }
    }
    return *instance;    
}

bool ParseReply(const char *buf, size_t count, const char *&parseEnd, FTPReply &reply)
{
    size_t   bytesRead = 0;
    size_t   freespace = count;
    size_t   bytesReadTotally = 0;  
    bool     isMultiLineReply = false;
    // 判断ftp回复是单行还是多行
    if (IsOneLineFormat(buf, count)) {
        isMultiLineReply = false;
    } else if (IsMultiLineFormat(buf, count)){
        isMultiLineReply = true;
    } else {
        // ftp服务器回复的格式错误
        parseEnd = nullptr;
        return false;
    }
    const char *matchEnd = nullptr;
    // ftp回复为一行格式
    if (!isMultiLineReply) {
        if (!IsMatchedOneLine(buf, count, matchEnd)) {
            parseEnd = nullptr;
            return false;    
        }
        std::memcpy(&reply.first, buf, 3);
        reply.first[3] = '\0';
        reply.second.assign(buf + 3, matchEnd);
        parseEnd = matchEnd;
        return true;
    }
    // ftp回复为多行格式
    if (IsMatchedMultiline(buf, count)) {
        if (!IsMatchedOneLine(buf, count, matchEnd)) {
            parseEnd = nullptr;
            return false;    
        }
        std::memcpy(&reply.first, buf, 3);
        reply.first[3] = '\0';
        reply.second.assign(buf + 3, matchEnd);
        parseEnd = matchEnd;
        return true;
    } else {
        parseEnd = nullptr;
        return false;
    }
}

} // namespace ftpclient