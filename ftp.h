#ifndef _FTP_H
#define _FTP_H

#include <vector>
#include <utility>
#include <string>
#include <sstream>
namespace ftpclient {

struct Reply
{
    char code[4];
    std::string detail;
};

struct DirContent 
{
    std::shared_ptr<std::vector<std::string>> content;
};

enum class TransmissonMode: size_t
{
    STREAM_MODE = 0, // default mode
    BLOCK_MODE = 1,
    COMPRESS_MODE = 2
};
enum class FileStructure: size_t
{
    FILE, // default file structure
    RECORD,
    PAGE

};
enum class FileType: size_t
{
    ASCII_TYPE, // default type
    EBCDIC_TYPE,
    IMAGE_TYPE,
    LOCAL_TYPE
};

static TransmissonMode StrToMode(const std::string &mode)
{
    static const std::string table[] = {"S", "B", "C"};
    for (size_t i = 0; i < 3; ++i) {
        if (table[i] == mode) {
            return static_cast<TransmissonMode>(i);
        }
    }
    return TransmissonMode::STREAM_MODE;
}

static FileStructure StrToStru(const std::string &stru)
{
    static const std::string table[] = {"F", "R", "P"};
    for (size_t i = 0; i < 3; ++i) {
        if (table[i] == stru) {
            return static_cast<FileStructure>(i);
        }
    }
    return FileStructure::FILE;

}

static FileType StrToFtype(const std::string &type)
{
    static const std::string table[] = {"A", "E", "I", "L"};
    for (size_t i = 0; i < 4; ++i) {
        if (table[i] == type) {
            return static_cast<FileType>(i);
        }
    }
    return FileType::ASCII_TYPE;
}

// enum值转换为对应的字符串值
static std::string ModeToStr(TransmissonMode mode)
{
    static const std::string table[] = {"S", "B", "C"};
    return table[static_cast<size_t>(mode)];
}

static std::string StruToStr(FileStructure stru)
{
    static const std::string table[] = {"F", "R", "P"};
    return table[static_cast<size_t>(stru)];

}

static std::string FtypeToStr(FileType type)
{
    static const std::string table[] = {"A", "E", "I", "L"};
    return table[static_cast<size_t>(type)];
}


class FTPClientConfig
{
public:
    static FTPClientConfig &GetInstance();
public:
    TransmissonMode mode = TransmissonMode::STREAM_MODE;
    FileStructure   structure = FileStructure::FILE;
    FileType        type = FileType::ASCII_TYPE;
protected:
    FTPClientConfig() = default;
};

template <typename T>
std::ostringstream& BuildCmdImpl(std::ostringstream &sout, const T &arg)
{
    sout << " " << arg;
    sout << "\r\n";
    return sout;
}

template <typename T, typename... Args>
std::ostringstream& BuildCmdImpl(std::ostringstream &sout, const T &arg, const Args&... args) {
    sout << " " << arg;
    return BuildCmdImpl(sout, args...);
}

std::string BuildCmd(const std::string &cmd);

template <typename T, typename... Args>
std::string BuildCmd(const std::string &cmd, const T &arg, const Args&... args) {
    std::ostringstream sout;
    sout << cmd;
    BuildCmdImpl(sout, arg, args...);
    return sout.str();
}

bool ParseReply(const char *buf, size_t count, const char *&parseEnd, Reply &reply);


} // namespace ftpclient

#endif // _FTP_H
