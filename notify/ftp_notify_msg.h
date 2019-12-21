#ifndef _FTP_NOTIFY_MSG_H
#define _FTP_NOTIFY_MSG_H

#include <cassert>
#include <string>
#include <memory>
#include <vector>


namespace ftpclient {
enum class MsgEnum 
{
    FTP_CMD_NETWORK_CONNECT,
    FTP_CMD_NETWORK_ERROR,
    FTP_CMD_NETWORK_CLOSED,
    FTP_REPLY,
    FTP_DIR_CONTENT,
    FTP_FILE_DOWNLOAD_START,
    FTP_FILE_DOWNLOAD_PROGRESS,
    FTP_FILE_DOWNLOAD_ERROR,
    FTP_FILE_DOWNLOAD_FINISH,
    FTP_FILE_UPLOAD_START,
    FTP_FILE_UPLOAD_PROGRESS,
    FTP_FILE_UPLOAD_ERROR,
    FTP_FILE_UPLOAD_FINISH
};

struct DataOfFTPReply
{
    char code[4];
    std::string detail;
};

struct DataOfFTPDirContent 
{
    std::string path;
    std::unique_ptr<char> buf;
    uint16_t  bufSize;
};

struct DataOfFTPCmdNetworkConnect
{
    std::string peerIP;
    uint16_t  peerPort;
};

struct DataOfFTPCmdNetworkError
{
    std::string errmsg;
};

template <class Path>
struct DataOfFTPFileDownloadStart
{
    std::string filenameOnServer;
    Path filenameOnLocal;
};

template <class Path>
struct DataOfFTPFileDownloadProgress
{
    std::string filenameOnServer;
    Path filenameOnLocal;
    uint64_t bytesRecv;
    uint64_t fileSize;
};

template <class Path>
struct DataOfFTPFileDownloadError
{
    std::string filenameOnServer;
    Path filenameOnLocal;
    std::string  errmsg;
};

template <class Path>
struct DataOfFTPFileDownloadFinish
{
    std::string filenameOnServer;
    Path filenameOnLocal;
};

template <class Path>
struct DataOfFTPFileUploadStart
{
    std::string filenameOnServer;
    Path filenameOnLocal;
};

template <class Path>
struct DataOfFTPFileUploadProgress
{
    std::string filenameOnServer;
    Path filenameOnLocal;
    uint64_t bytesRecv;
    uint64_t fileSize;
};

template <class Path>
struct DataOfFTPFileUploadError
{
    std::string filenameOnServer;
    Path filenameOnLocal;
    std::string  errmsg;
};

template <class Path>
struct DataOfFTPFileUploadFinish
{
    std::string filenameOnServer;
    Path filenameOnLocal;
};


struct Notify
{
    MsgEnum msg;
    void   *data;
};

template <MsgEnum val>
struct EnumMapMsgType;

template <>
struct EnumMapMsgType<MsgEnum::FTP_DIR_CONTENT>
{
    using DataType = DataOfFTPDirContent;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_ERROR>
{
    template <class Path>
    using DataType =   DataOfFTPFileDownloadError<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_FINISH>
{
    template <class Path>
    using DataType = DataOfFTPFileDownloadFinish<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>
{
    template <class Path>
    using DataType = DataOfFTPFileDownloadProgress<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_START>
{
    template <class Path>
    using DataType = DataOfFTPFileDownloadStart<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_ERROR>
{
    template <class Path>
    using DataType = DataOfFTPFileUploadError<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_FINISH>
{
    template <class Path>
    using DataType = DataOfFTPFileUploadFinish<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_PROGRESS>
{
    template <class Path>
    using DataType = DataOfFTPFileUploadProgress<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_START>
{
    template <class Path>
    using DataType = DataOfFTPFileUploadStart<Path>;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_REPLY>
{
    using DataType = DataOfFTPReply;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_CMD_NETWORK_CLOSED>
{

};

template <>
struct EnumMapMsgType<MsgEnum::FTP_CMD_NETWORK_CONNECT>
{
    using DataType = DataOfFTPCmdNetworkConnect;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_CMD_NETWORK_ERROR>
{
    using DataType = DataOfFTPCmdNetworkError;
};


template <MsgEnum type, class DataType = EnumMapMsgType<type>::DataType>
std::shared_ptr<DataType> Extract(Notify notify)
{
    assert(notify.msg == type);
    return std::shared_ptr<DataType>(
            reinterpret_cast<DataType *>(notify.data));
}

template <class Path, MsgEnum type, template <class> class DataType = EnumMapMsgType<type>::DataType>
std::shared_ptr<DataType<Path>> ExtractWithPath(Notify notify)
{
    assert(notify.msg == type);
    return std::shared_ptr<DataType<Path>>(
            reinterpret_cast<DataType<Path> *>(notify.data));
}

constexpr auto ExtractDataOfFTPReply                 = &Extract<MsgEnum::FTP_REPLY>                 ;
constexpr auto ExtractDataOfFTPDirContent            = &Extract<MsgEnum::FTP_DIR_CONTENT>           ;
constexpr auto ExtractDataOfFTPCmdNetworkConnect     = &Extract<MsgEnum::FTP_CMD_NETWORK_CONNECT>   ;
constexpr auto ExtractDataOfFTPCmdNetworkError       = &Extract<MsgEnum::FTP_CMD_NETWORK_ERROR>     ;

template <class Path>
constexpr auto ExtractDataOfFTPFileDownloadStart     = &ExtractWithPath<Path, MsgEnum::FTP_FILE_DOWNLOAD_START>   ;
template <class Path>
constexpr auto ExtractDataOfFTPFileDownloadProgress  = &ExtractWithPath<Path, MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>;
template <class Path>
constexpr auto ExtractDataOfFTPFileDownloadError     = &ExtractWithPath<Path, MsgEnum::FTP_FILE_DOWNLOAD_ERROR>   ;
template <class Path>
constexpr auto ExtractDataOfFTPFileDownloadFinish    = &ExtractWithPath<Path, MsgEnum::FTP_FILE_DOWNLOAD_FINISH>  ;
template <class Path>
constexpr auto ExtractDataOfFTPFileUploadStart       = &ExtractWithPath<Path, MsgEnum::FTP_FILE_UPLOAD_START>     ;
template <class Path>
constexpr auto ExtractDataOfFTPFileUploadProgress    = &ExtractWithPath<Path, MsgEnum::FTP_FILE_UPLOAD_PROGRESS>  ;
template <class Path>
constexpr auto ExtractDataOfFTPFileUploadError       = &ExtractWithPath<Path, MsgEnum::FTP_FILE_UPLOAD_ERROR>     ;
template <class Path>
constexpr auto ExtractDataOfFTPFileUploadFinish      = &ExtractWithPath<Path, MsgEnum::FTP_FILE_UPLOAD_FINISH>    ;

constexpr auto ExtractDataOfFTPFileDownloadStartA     = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_DOWNLOAD_START>   ;
constexpr auto ExtractDataOfFTPFileDownloadProgressA  = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>;
constexpr auto ExtractDataOfFTPFileDownloadErrorA     = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_DOWNLOAD_ERROR>   ;
constexpr auto ExtractDataOfFTPFileDownloadFinishA    = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_DOWNLOAD_FINISH>  ;
constexpr auto ExtractDataOfFTPFileUploadStartA       = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_UPLOAD_START>     ;
constexpr auto ExtractDataOfFTPFileUploadProgressA    = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_UPLOAD_PROGRESS>  ;
constexpr auto ExtractDataOfFTPFileUploadErrorA       = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_UPLOAD_ERROR>     ;
constexpr auto ExtractDataOfFTPFileUploadFinishA      = &ExtractWithPath<std::string, MsgEnum::FTP_FILE_UPLOAD_FINISH>    ;

constexpr auto ExtractDataOfFTPFileDownloadStartW     = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_DOWNLOAD_START>   ;
constexpr auto ExtractDataOfFTPFileDownloadProgressW  = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>;
constexpr auto ExtractDataOfFTPFileDownloadErrorW     = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_DOWNLOAD_ERROR>   ;
constexpr auto ExtractDataOfFTPFileDownloadFinishW    = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_DOWNLOAD_FINISH>  ;
constexpr auto ExtractDataOfFTPFileUploadStartW       = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_UPLOAD_START>     ;
constexpr auto ExtractDataOfFTPFileUploadProgressW    = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_UPLOAD_PROGRESS>  ;
constexpr auto ExtractDataOfFTPFileUploadErrorW       = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_UPLOAD_ERROR>     ;
constexpr auto ExtractDataOfFTPFileUploadFinishW      = &ExtractWithPath<std::wstring, MsgEnum::FTP_FILE_UPLOAD_FINISH>    ;


}// namespace ftpclient

#endif // _FTP_NOTIFY_MSG_H
