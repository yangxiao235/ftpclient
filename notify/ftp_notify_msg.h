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
    assert(notify.type == type);
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

template <MsgEnum type = MsgEnum::FTP_REPLY, class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType> (*ExtractDataOfFTPReply)(Notify) 
        =  &Extract<type>;

template <MsgEnum type = MsgEnum::FTP_DIR_CONTENT>
constexpr std::shared_ptr<typename EnumMapMsgType<type>::DataType> (*ExtractDataOfFTPDirContent)(Notify) 
        = &Extract<type>;

template <MsgEnum type = MsgEnum::FTP_CMD_NETWORK_CONNECT, class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType> (*ExtractDataOfFTPCmdNetworkConnect)(Notify) 
        =  &Extract<type>;

template <MsgEnum type = MsgEnum::FTP_CMD_NETWORK_ERROR>
constexpr std::shared_ptr<typename EnumMapMsgType<type>::DataType> (*ExtractDataOfFTPCmdNetworkError)(Notify)
        = &Extract<type>;

template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadStart)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadStartA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadStartW)(Notify) 
            =  &ExtractWithPath<Path, type>;


template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadProgress)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadProgressA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadProgressW)(Notify) 
            =  &ExtractWithPath<Path, type>;


template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadError)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadErrorA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadErrorW)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadFinish)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadFinishA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_DOWNLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileDownloadFinishw)(Notify) 
            =  &ExtractWithPath<Path, type>;


template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadStart)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadStartA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_START, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadStartW)(Notify) 
            =  &ExtractWithPath<Path, type>;


template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadProgress)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadProgressA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_PROGRESS, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadProgressW)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadError)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadErrorA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_ERROR, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadErrorW)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadFinish)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::string, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadFinishA)(Notify) 
            =  &ExtractWithPath<Path, type>;

template <class Path = std::wstring, 
          MsgEnum type = MsgEnum::FTP_FILE_UPLOAD_FINISH, 
          template <class> class DataType = EnumMapMsgType<type>::DataType>
constexpr std::shared_ptr<DataType<Path>> (*ExtractDataOfFTPFileUploadFinishW)(Notify) 
            =  &ExtractWithPath<Path, type>;

}// namespace ftpclient

#endif // _FTP_NOTIFY_MSG_H
