#include "ftp.h"
#include <mutex>
#include <regex>

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

std::string BuildCmd(const std::string &cmd) {
    std::ostringstream sout;
    sout << cmd;
    sout << "\r\n";
    return sout.str();
}    

FTPClientConfig &FTPClientConfig::GetInstance()
{
    static FTPClientConfig *instance = nullptr;
    if (!instance) {
        static std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new FTPClientConfig();
        }
    }
    return *instance;    
}

bool ParseReply(const char *buf, size_t count, const char *&parseEnd, Reply &reply)
{
    size_t   bytesRead = 0;
    size_t   freespace = count;
    size_t   bytesReadTotally = 0;  
    bool     isMultiLineReply = false;
    // 判断ftp回复是单行还是多�?
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
    // ftp回复为一行格�?
    if (!isMultiLineReply) {
        if (!IsMatchedOneLine(buf, count, matchEnd)) {
            parseEnd = nullptr;
            return false;    
        }
        std::memcpy(&reply.code, buf, 3);
        reply.code[3] = '\0';
        reply.detail.assign(buf, matchEnd);
        parseEnd = matchEnd;
        return true;
    } else  {
        if (!IsMatchedMultiline(buf, count, matchEnd)) {
            parseEnd = nullptr;
            return false;    
        }
        std::memcpy(&reply.code, buf, 3);
        reply.code[3] = '\0';
        reply.detail.assign(buf, matchEnd);
        parseEnd = matchEnd;
        return true;
    }
}

} // namespace ftpclient