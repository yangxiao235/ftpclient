#ifndef _FTP_NOTIFY_MSG_H
#define _FTP_NOTIFY_MSG_H

#include "ftpclient/platform_specific.h"
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
    FTP_CMD_RECV_DATA,
    FTP_DIR_CONTENT,
    FTP_GET_DIR_CONTENT_ERROR,
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

struct DataOfFTPCmdRecvData
{
    std::string data;
};

struct DataOfFTPDirContent 
{
    std::string path;
    std::unique_ptr<char []> buf;
    size_t  bufSize;
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

struct DataOfFTPFileDownloadStart
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
};

struct DataOfFTPFileDownloadProgress
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
    uint64_t bytesRecv;
    uint64_t fileSize;
};

struct DataOfFTPFileDownloadError
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
    std::string  errmsg;
};

struct DataOfFTPFileDownloadFinish
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
};

struct DataOfFTPFileUploadStart
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
};

struct DataOfFTPFileUploadProgress
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
    uint64_t bytesSend;
    uint64_t fileSize;
};

struct DataOfFTPFileUploadError
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
    std::string  errmsg;
};

struct DataOfFTPFileUploadFinish
{
    std::string filenameOnServer;
    FTPClientPathType filenameOnLocal;
};

struct DataOfFTPGetDirContentError
{
    std::string errmsg;
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
    using DataType =   DataOfFTPFileDownloadError;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_FINISH>
{
    using DataType = DataOfFTPFileDownloadFinish;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>
{
    using DataType = DataOfFTPFileDownloadProgress;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_DOWNLOAD_START>
{
    using DataType = DataOfFTPFileDownloadStart;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_ERROR>
{
    using DataType = DataOfFTPFileUploadError;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_FINISH>
{
    using DataType = DataOfFTPFileUploadFinish;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_PROGRESS>
{
    using DataType = DataOfFTPFileUploadProgress;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_FILE_UPLOAD_START>
{
    using DataType = DataOfFTPFileUploadStart;
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

template <>
struct EnumMapMsgType<MsgEnum::FTP_GET_DIR_CONTENT_ERROR>
{
    using DataType = DataOfFTPGetDirContentError;
};

template <>
struct EnumMapMsgType<MsgEnum::FTP_CMD_RECV_DATA>
{
    using DataType = DataOfFTPCmdRecvData;
}; 

template <MsgEnum type, class DataType = EnumMapMsgType<type>::DataType>
std::shared_ptr<DataType> Extract(Notify notify)
{
    assert(notify.msg == type);
    return std::shared_ptr<DataType>(
            reinterpret_cast<DataType *>(notify.data));
}

constexpr auto ExtractDataOfFTPReply                 = &Extract<MsgEnum::FTP_REPLY>                 ;
constexpr auto ExtractDataOfFTPDirContent            = &Extract<MsgEnum::FTP_DIR_CONTENT>           ;
constexpr auto ExtractDataOfFTPCmdNetworkConnect     = &Extract<MsgEnum::FTP_CMD_NETWORK_CONNECT>   ;
constexpr auto ExtractDataOfFTPCmdNetworkError       = &Extract<MsgEnum::FTP_CMD_NETWORK_ERROR>     ;
constexpr auto ExtractDataOfFTPFileDownloadStart     = &Extract<MsgEnum::FTP_FILE_DOWNLOAD_START>   ;
constexpr auto ExtractDataOfFTPFileDownloadProgress  = &Extract<MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>;
constexpr auto ExtractDataOfFTPFileDownloadError     = &Extract<MsgEnum::FTP_FILE_DOWNLOAD_ERROR>   ;
constexpr auto ExtractDataOfFTPFileDownloadFinish    = &Extract<MsgEnum::FTP_FILE_DOWNLOAD_FINISH>  ;
constexpr auto ExtractDataOfFTPFileUploadStart       = &Extract<MsgEnum::FTP_FILE_UPLOAD_START>     ;
constexpr auto ExtractDataOfFTPFileUploadProgress    = &Extract<MsgEnum::FTP_FILE_UPLOAD_PROGRESS>  ;
constexpr auto ExtractDataOfFTPFileUploadError       = &Extract<MsgEnum::FTP_FILE_UPLOAD_ERROR>     ;
constexpr auto ExtractDataOfFTPFileUploadFinish      = &Extract<MsgEnum::FTP_FILE_UPLOAD_FINISH>    ;
constexpr auto ExtractDataOfFTPCmdRecvData           = &Extract<MsgEnum::FTP_CMD_RECV_DATA>    ;


// 构建消息附属数据的帮助例程
static Notify BuildCmdNetworkConnect(const std::string &peerIP, uint16_t peerPort)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_CMD_NETWORK_CONNECT;
    auto data = new DataOfFTPCmdNetworkConnect;
    data->peerIP = peerIP;
    data->peerPort = peerPort;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildCmdNetworkError(const std::string &errmsg)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_CMD_NETWORK_ERROR;
    auto data = new DataOfFTPCmdNetworkError;
    data->errmsg = errmsg;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildCmdNetworkClosed()
{
    Notify notify;
    notify.msg = MsgEnum::FTP_CMD_NETWORK_CLOSED;
    notify.data = nullptr;
    return notify;
}
static Notify BuildFTPReply(const DataOfFTPReply &reply)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_REPLY;
    auto data = new DataOfFTPReply;
    *data = reply;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildDirContent(const std::string &dir, std::unique_ptr<char[]> buf, size_t bufSize)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_DIR_CONTENT;
    auto data = new DataOfFTPDirContent;
    data->path = dir;
    data->buf = std::move(buf);
    data->bufSize = bufSize;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildDownloadStart(const std::string &filenameOnServer, const FTPClientPathType &filenameOnLocal)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_DOWNLOAD_START;
    auto data = new DataOfFTPFileDownloadStart;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildDownloadProgress(const std::string &filenameOnServer, 
                                              const FTPClientPathType &filenameOnLocal,
                                              uint64_t bytesRecv,
                                              uint64_t fileSize)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS;
    auto data = new DataOfFTPFileDownloadProgress;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    data->bytesRecv = bytesRecv;
    data->fileSize = fileSize;    
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildDownloadError(const std::string &filenameOnServer, 
                                        const FTPClientPathType &filenameOnLocal,
                                        const std::string &errmsg)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_DOWNLOAD_ERROR;
    auto data = new DataOfFTPFileDownloadError;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    data->errmsg = errmsg;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildDownloadFinish(const std::string &filenameOnServer, const FTPClientPathType &filenameOnLocal)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_DOWNLOAD_FINISH;
    auto data = new DataOfFTPFileDownloadFinish;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}

static Notify BuildUploadStart(const std::string &filenameOnServer, const FTPClientPathType &filenameOnLocal)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_UPLOAD_START;
    auto data = new DataOfFTPFileUploadStart;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildUploadProgress(const std::string &filenameOnServer, 
                                              const FTPClientPathType &filenameOnLocal,
                                              uint64_t byteSend,
                                              uint64_t fileSize)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_UPLOAD_PROGRESS;
    auto data = new DataOfFTPFileUploadProgress;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    data->bytesSend = byteSend;
    data->fileSize = fileSize;    
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildUploadError(const std::string &filenameOnServer, 
                                        const FTPClientPathType &filenameOnLocal,
                                        const std::string &errmsg)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_UPLOAD_ERROR;
    auto data = new DataOfFTPFileUploadError;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    data->errmsg = errmsg;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
static Notify BuildUploadFinish(const std::string &filenameOnServer, const FTPClientPathType &filenameOnLocal)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_FILE_UPLOAD_FINISH;
    auto data = new DataOfFTPFileUploadFinish;
    data->filenameOnServer = filenameOnServer;
    data->filenameOnLocal = filenameOnLocal;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}

static Notify BuildGetDirContentError(const std::string &errmsg)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_GET_DIR_CONTENT_ERROR;
    auto data = new DataOfFTPGetDirContentError;
    data->errmsg = errmsg;
    notify.data = reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}

static Notify BuildCmdRecvData(const std::string &text)
{
    Notify notify;
    notify.msg = MsgEnum::FTP_CMD_RECV_DATA;
    auto data = new DataOfFTPCmdRecvData;
    data->data = text;
    notify.data =  reinterpret_cast <void *>(data);
    data = nullptr;
    return notify;
}
// 释放Notify的void *成员占用的资源
void ReleaseNotify(Notify &notify);


}// namespace ftpclient

#endif // _FTP_NOTIFY_MSG_H
