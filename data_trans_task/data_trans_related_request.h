#ifndef _DATA_TRANS_RELATED_REQUEST_H
#define _DATA_TRANS_RELATED_REQUEST_H

#include "ftpclient/config.h"
#include "ftpclient/common_types.h"
#include "ftpclient/notify/ftp_notify_msg.h"
#include "data_trans_request.h"
#include "data_trans_request_pending_list.h"
#include <asio.hpp>
#include <glog/logging.h>
#include <chrono>
#include <windows.h>

namespace ftpclient {

using asio::windows::random_access_handle;

// 下载文件的实现思路
// -------------------
//    以网络接收数据驱动写文件操作, 每次有网络数据到来时,
// 调用异步写文件操作将数据写入文件. 在实现上, 利用单个缓冲区
// 同时进行读写, 避免数据被多次复制. 
//    比较困难的地方是, 由于读网络数据和写数据到文件
// 都是异步的, 且每次写入缓冲区, 写起始地址m_writePos都在向前递增,
// 这将导致最终m_writePos处于缓冲区结束位置. 在m_writePos到达结尾时
// 将重新把数据区段移到缓冲区头部. 如果异步操作未完成, 那么缓冲区
// 数据将不能被移动.
//    为了解决这个问题, 在缓冲区空间不足时, 阻塞网络读数据操作. 这样的话
// 就不能实现网络读取驱动文件写入了. 所以设置m_blockNetworkRead标志, 
// 当异步写文件完成时, 如果网络读数据被阻塞, 这时可重新安排缓冲区空间, 
// 然后开始网络读数据操作.
//
// 文件保存
// --------
// 文件如果下载出现错误, 可以简单的将不完整的文件删除掉. 或者一开始就命令为
// 临时文件, 在完成时将其改为目标文件, 如果下载出现错误, 则保存为临时文件.
// FileSavePolicy要求的接口为:
//      HANDLE OpenFile(const FTPClientPathType &);
//      SaveFileOnSuccess();
//      SaveFileOnFailure();
// 注意: FileSavePolicy不能管理HANDLE, 管理的工作交给"客户"完成.
//
// 消息通知
// --------
// 在特定节点上需要向"用户端"通知发生的消息. 可以实现多种消息通知机制.
// MsgNotify要求的接口为:
//  ReportEvent(const Notify &)
// 进展通知:
// --------
// 用户可以选择不通知下载进展或者通知下载进展
// ReportProgressPolicy<ReportedObject>要求的接口为:
//   ReportProgressPolicy(io_context &);
//   Start(const ReportedObject &, size_t duration); // duration为毫秒值
//   Stop()
// 对ReportedObject的要求的接口为:
//   void Report();
//
template <class FileSavePolicy, class MsgNotify>
class DownloadFileRequest : public DataTransRequest, 
                                 public FileSavePolicy, 
                                 public MsgNotify
{
public:
    // 主动连接服务器
    DownloadFileRequest(
                        IOContextPointer io, 
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer, 
                        const std::string &ip,  // 服务器ip
                        uint16_t port, // 服务器端口
                        uint64_t  fileSize = 0,  // 文件大小
                        size_t  interval = 500, // 进度报告时间间隔, 以ms计算
                        uint16_t bufSize = 32768 ) // 接收缓冲区大小(32KiB默认)
        
        :m_io(io),
        m_filename(filename),
        m_filenameOnServer(filenameOnServer),
        m_ip(ip),
        m_port(port),
        m_socket(*m_io),
        m_file(*m_io),
        m_timer(*m_io),
        m_reportInterval(interval),
        m_bufSize(bufSize),
        m_passiveMode(false),
        m_fileSize(fileSize)
    {}
                        
    // 监听端口, 等待服务器连接
    DownloadFileRequest(
                        IOContextPointer io,
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer, 
                        uint16_t port,
                        uint64_t fileSize = 0,
                        size_t interval = 500,
                        size_t bufSize = 32768 )
        :m_io(io),
        m_filename(filename),
        m_filenameOnServer(filenameOnServer),
        m_port(port),
        m_socket(*m_io),
        m_file(*m_io),
        m_timer(*m_io),
        m_reportInterval(interval),
        m_bufSize(bufSize),
        m_passiveMode(true),
        m_fileSize(fileSize),
    {}
public:
    void Start() override
    {
        LOG(INFO) << "DownloadFileRequest::Start()";
        if (m_passiveMode) {
            m_acceptor = std::move(std::make_unique<tcp::acceptor>(*m_io));
            m_acceptor->open(tcp::v4());
            m_acceptor->bind(tcp::endpoint(tcp::v4(), m_port));
            m_acceptor->async_accept(m_socket, std::bind(&DownloadFileRequest::AcceptHandler,
                                        this, std::placeholders::_1));
            return;
        }
        m_socket.async_connect(tcp::endpoint(address::from_string(m_ip), m_port), 
                                 std::bind(&DownloadFileRequest::ConnectHandler, 
                                           this, std::placeholders::_1));
    }
    bool Complete() override
    {
        return m_complete;
    }
    void Destroy() override
    {
        if (!m_complete) {
            // 用户可能在文件还未打开或者文件传送尚未完成的情况下手动
            // 终止线程
            SaveFileOnFailure();
        } 
        if (m_socket.is_open()) {
            m_socket.close();
        }
        if (m_acceptor && m_acceptor->is_open()) {
            m_acceptor->close();
        }
        delete this;
    }

public:    
    void ConnectHandler(const asio::error_code &error)
    {
        if (!error) {
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "DownloadFileRequest::ConnectHandler() failed: server(" << m_ip << ":" <<m_port
                       << "), " <<  error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void AcceptHandler(const asio::error_code& error)
    {
        if (!error) {
            // 不允许建立多个tcp连接
            m_acceptor->close();
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "DownloadFileRequest::AcceptHandler() failed: " << "host port(" << m_port << ")," 
                       << error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void ReadHandler(const asio::error_code &error, size_t bytesRead)
    {
        if (!error) {
            m_writePos += bytesRead;
            if (bytesRead == 0) {
                // 数据接收完成
                m_socket.close();
                // 此时m_complete不会立即设置, 需要等待
                // 文件异步写入完成才设置
                m_dataTransComplete = true; 
                return;
            }
            if (m_lastWriteFileFinished) {
                // 上一次异步写文件已经完成, 可以开始继续写文件
                bool callAsyncWrite = false;
                m_file.async_write_some_at(m_bytesHasBeenWriteToFile, asio::buffer(m_readPos, ReadableSpaceSize()), 
                       std::bind(&DownloadFileRequest::WriteFileHandler, this, std::placeholders::_1, 
                        std::placeholders::_2));
                m_lastWriteFileFinished = true;
            }
            if (WritableSpaceSize() < kWriteMinWindowSize) {
                m_blockNetworkRead = true;
            } else {
                ReadFromNetwork();    
            }
        } else {
            if (error ==  asio::error_code{asio::error::eof, asio::error::get_misc_category()}) {
                LOG(INFO) << "TCP connection has been closed";
                m_dataTransComplete = true;
                if (m_lastWriteFileFinished) {
                    m_complete = true;
                    OnComplete();
                }
            } else {
                LOG(ERROR) << "DownloadFileRequest::ReadHandler() failed: " << error.message() << ", error code"
                           << error.value();
                ReleaseOnError(error.message());
            }
        }
    }

    void WriteFileHandler( const asio::error_code& error, std::size_t bytesWrite)
    {
        if (!error) {
            m_bytesHasBeenWriteToFile += bytesWrite;
            m_readPos += bytesWrite;
            if (ReadableSpaceSize() && m_dataTransComplete) {
                // 文件写入完成
                OnComplete();
                return;
            }
            m_lastWriteFileFinished = true;
            if (m_blockNetworkRead) {
                m_blockNetworkRead = false;
                RecycleBuffer();
                ReadFromNetwork();
            }
        } else {
            LOG(ERROR) << "DownloadFileRequest::WriteHandler() failed: " << error.message() << ", error code "
                       << error.value();
            ReleaseOnError(error.message());
        }
    }

    void TimerHandler(const asio::error_code &error)
    {
        if (!error) {
            ReportProgress();
            SetupTimerAndStart();
        } else {
            LOG(ERROR) << "DownloadFileRequest::TimerHandler failed: "<< error.message() 
                       << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
protected:
    ~DownloadFileRequest() = default;

    void AllocateBuffer()
    {
        if (!m_buf) {
            m_buf = std::move(std::unique_ptr<char []>(new char[m_bufSize]));
            m_readPos = m_writePos = &m_buf[0];
            m_bufEndPos = &m_buf[0] + m_bufSize;
        }    
    }
    void RecycleBuffer()
    {
        std::memmove(m_buf.get(), m_readPos, ReadableSpaceSize());
        m_writePos = m_buf.get() + ReadableSpaceSize();
        m_readPos = m_buf.get();
        if (WritableSpaceSize() < kWriteMinWindowSize) {
            // 文件数据没有被写入, 严重错误
            LOG(FATAL) << "DownloadFileRequest::RecycleBuffer(): after move buffer, still no free buffer space to use!";
        }
    }
    void OpenFile()
    {
        // FileSavePolicy不保存文件句柄
        // 仅保存文件名和可能实现临时文件的临时文件名
        // 这里返回的句柄有用户负责管理
        m_file.assign(FileSavePolicy::OpenFile(m_filename));
    }
    void ReportProgress()
    {
        ReportDownloadProgress(m_filenameOnServer, m_filename, m_bytesHasBeenWriteToFile, m_fileSize);
    }

    void SetupTimerAndStart()
    {
        m_timer.expires_after(std::chrono::milliseconds{m_reportInterval});
        m_timer.async_wait(std::bind(&DownloadFileRequest::TimerHandler, this, std::placeholders::_1));
    }

    void ReadFromNetwork()
    {
        assert(WritableSpaceSize() !=  0);
        m_socket.async_read_some(asio::buffer(m_writePos, WritableSpaceSize()),
                    std::bind(&DownloadFileRequest::ReadHandler, 
                             this, std::placeholders::_1, std::placeholders::_2));
    }
    void StartReadFromNetwork()
    {
        ReadFromNetwork();
    }
    void InitWhenTransBegin()
    {
        ReportDownloadStart(m_filenameOnServer, m_filename);
        SetupTimerAndStart();
        OpenFile();
        AllocateBuffer();
        StartReadFromNetwork();
        
    }
    void ReleaseOnError(const std::string &errmsg)
    {
        ReportDownloadError(m_filenameOnServer, m_filename, errmsg);
        m_socket.close();
        SaveFileOnFailure();
        if (m_acceptor) {
            m_acceptor->close();
        }
    }

    size_t WritableSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_bufEndPos - m_writePos);
    }

    size_t ReadableSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_writePos - m_readPos);
    }

    size_t RecycleSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_readPos - m_buf.get());
    }

    void OnComplete()
    {
        ReportDownloadFinish(m_filenameOnServer, m_filename);
        if (m_fileSize != 0) {
            if (m_fileSize == m_bytesHasBeenWriteToFile) {
                SaveFileOnSuccess();
            } else {
                SaveFileOnFailure();
            }
        } else {
            // 下载文件前不知道文件大小的话, 默认是完整数据
            SaveFileOnSuccess();
        }
        // 请求已经完成
        m_complete = true;
    }
private:
    // 储存到本地的文件名, 
    // 要求: (1) 路径全名; (2) 文件不存在
    FTPClientPathType       m_filename;
    // 服务器上下载的文件名(全路径或者相对路径)
    std::string m_filenameOnServer;
    // 主动模式下为服务器ip, 被动模式下无意义
    std::string m_ip; 
    // 主动模式下为服务器port, 被动模式下为监听端口
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    // 文件下载是否完成
    bool        m_complete = false;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket     m_socket;
    random_access_handle m_file;
    // 数据缓冲区
    // 从网络写数据的缓冲区最小窗口大小
    // 如果缓冲区空闲容量小于它, 则需要将m_writePos和m_readPos
    // 重新进行规整
    static constexpr size_t kWriteMinWindowSize = 256;
    std::unique_ptr<char[]> m_buf;
    std::size_t    m_bufSize;
    // 网络接收到数据向缓冲区写入的起始位置
    char    *m_writePos = nullptr;
    // 写文件时的起始位置
    char     *m_readPos  = nullptr;
    // 缓冲区结束位置[0, m_bufEndPos)
    char     *m_bufEndPos = nullptr;    
    // 上一次异步写文件是否完成?
    bool             m_lastWriteFileFinished = true;
    // 由于网络写数据缓冲区不足, 暂时阻塞从网络读取数据
    bool             m_blockNetworkRead = false;
    // 网络数据传输是否完成(eof)
    bool             m_dataTransComplete = false;
    // 在客户端被动连接模式下, 表示是否
    // 已经和服务器建立了连接(仅允许和服务器建立一个连接)
    bool             m_accepted = false;
    // 已经写入文件的字节数
    uint64_t         m_bytesHasBeenWriteToFile = 0;
    // 文件大小
    // 如果是可选项的话, m_fileSize为0
    uint64_t         m_fileSize;
    // 进度报告间隔, 按照ms计算
    size_t           m_reportInterval;
    // 
    asio::steady_timer m_timer;
    
};

// UploadFileRequest和DownloadFileRequest实现类似. 
// 这里的文件读取操作相当于DownloadFileRequest的网络文件读取操作,
// 这里的写网络数据操作相当于DownloadFileRequest的写文件操作.
template <class MsgNotifyPolicy>
class UploadFileRequest : public DataTransRequest, public MsgNotifyPolicy
{
public:
    // 主动连接服务器
    UploadFileRequest(
                        IOContextPointer io, 
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer,
                        const std::string &ip,  // 服务器ip
                        uint16_t port, // 服务器端口
                        size_t  interval = 500, // 每500ms通知一次进度
                        uint16_t bufSize = 32768) // 发送缓冲区大小(32KiB默认)
        :m_io(io),
        m_socket(*io),
        m_filename(filename),
        m_filenameOnServer(filenameOnServer),
        m_file(*io),
        m_ip(ip),
        m_port(port),
        m_passiveMode(false),
        m_bufSize(bufSize),
        m_reportInterval(interval),
        m_timer(*io)
    {}
       
    // 监听端口, 等待服务器连接
    UploadFileRequest(
                        IOContextPointer io,
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer,
                        uint16_t port,
                        size_t interval = 500,
                        uint16_t bufSize = 32768)
        :m_io(io),
        m_socket(*io),
        m_file(io),
        m_filename(filename),
        m_filenameOnServer(filenameOnServer),
        m_port(port),
        m_passiveMode(true),
        m_reportInterval(interval),
        m_timer(*io),
        m_bufSize(bufSize)
    {}
                        
public:
    void Start() override
    {
        LOG(INFO) << "UploadFileRequest::Start()";
        if (m_passiveMode) {
            m_acceptor = std::move(std::make_unique<tcp::acceptor>(*m_io));
            m_acceptor->open(tcp::v4());
            m_acceptor->bind(tcp::endpoint(tcp::v4(), m_port));
            m_acceptor->async_accept(m_socket, std::bind(&UploadFileRequest::AcceptHandler,
                                        this, std::placeholders::_1));
            return;
        }
        m_socket.async_connect(tcp::endpoint(address::from_string(m_ip), m_port), 
                                 std::bind(&UploadFileRequest::ConnectHandler, 
                                           this, std::placeholders::_1));
    }
    bool Complete() override
    {
        return m_complete;
    }
    void Destroy() override
    {
        if (!m_complete) {
            // 用户可能在文件还未打开或者文件传送尚未完成的情况下手动
            // 终止线程
            if (m_file.is_open()) {
                m_file.close();
            }
        } 
        if (m_socket.is_open()) {
            m_socket.close();
        }
        if (m_acceptor && m_acceptor->is_open()) {
            m_acceptor->close();
        }
        delete this;
    }
public:    
    void ConnectHandler(const asio::error_code &error)
    {
        if (!error) {
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "UploadFileRequest::ConnectHandler() failed: server(" << m_ip << ":" <<m_port
                       << "), " <<  error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void AcceptHandler(const asio::error_code& error)
    {
        if (!error) {
            m_acceptor->close();
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "UploadFileRequest::AcceptHandler() failed: " << "host port(" << m_port << ")," 
                       << error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void ReadFileHandler(const asio::error_code &error, size_t bytesRead)
    {
        if (!error) {
            m_writePos += bytesRead;
            m_bytesHasBeenReadFromFile += bytesRead;
            m_lastReadFileFinished = true;
            if (bytesRead == 0) {
                // 文件已读完
                m_file.close();
                // 此时m_complete不会立即设置, 需要等待
                // 网络传输完成才设置
                m_fileReadFinished = true; 
                return;
            }
            if (m_lastWriteNetworkFinished) {
                m_socket.async_write_some(asio::buffer(m_readPos, ReadableSpaceSize()),
                            std::bind(&UploadFileRequest::WriteHandler,
                                    this, std::placeholders::_1, std::placeholders::_2));
                m_lastAsycWriteInBytes = ReadableSpaceSize();
                m_lastWriteNetworkFinished = false;       
            }
            if (WritableSpaceSize() < kMinFreeWriteSapceSize) {
                m_blockFileRead = true;
            } else {
                ReadFromFile();
            }
        }else {
            if (error ==  asio::error_code{asio::error::eof, asio::error::get_misc_category()}) {
                LOG(INFO) << "Reading file has been reached to eof!";
                m_fileReadFinished = true;
                if (m_lastWriteNetworkFinished) {
                    m_complete = true;
                    OnComplete();
                }
            } else {
                ReportUploadError(m_filenameOnServer, m_filename, error.message());
                LOG(ERROR) << "UploadFileRequest::ReadHandler() failed: " << error.message() << ", error code"
                           << error.value();
                ReleaseOnError(error.message());
            }
        }
    }

    void WriteHandler( const asio::error_code& error, std::size_t bytesWrite)
    {
        if (!error) {
            m_lastWriteNetworkFinished = true;
            m_readPos += bytesWrite;
            m_bytesHasBeenSend += bytesWrite;
            if (m_lastAsycWriteInBytes != bytesWrite) {
                LOG(ERROR) << "Only part of data was send!";
            }
            if ((ReadableSpaceSize() == 0) && m_fileReadFinished) {
                // 文件写入完成
                OnComplete();
                return;
            }
            if (m_blockFileRead) {
                m_blockFileRead = false;
                RecycleBuffer();
                ReadFromFile();
            }
        } else {
            LOG(ERROR) << "UploadFileRequest::WriteHandler() failed: " << error.message() << ", error code "
                       << error.value();
            ReleaseOnError(error.message());
        }
    }

    void TimerHandler(const asio::error_code &error)
    {
        if (!error) {
            ReportProgress();
            SetupTimerAndStart();
        } else {
            LOG(ERROR) << "UploadFileRequest::TimerHandler failed: "<< error.message() 
                       << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
protected:  
    ~UploadFileRequest() = default;
    void AllocateBuffer()
    {
        if (!m_buf) {
            m_buf = std::move(std::unique_ptr<char []>(new char[m_bufSize]));
            m_readPos = m_writePos = &m_buf[0];
            m_bufEndPos = &m_buf[0] + m_bufSize;
        }    
    }
    void RecycleBuffer()
    {
        std::memmove(m_buf.get(), m_readPos, ReadableSpaceSize());
        m_writePos = m_buf.get() + ReadableSpaceSize();
        m_readPos = m_buf.get();
        if (WritableSpaceSize() < kMinFreeWriteSapceSize) {
            // 文件数据没有被写入, 严重错误
            LOG(FATAL) << "UploadFileRequest::RecycleBuffer(): after move buffer, still no free buffer space to use!";
        }
    }
    void OpenFile()
    {
#if defined(USE_CHAR_PATH)        
        auto fileHandle = ::CreateFileA(m_filename.c_str(),                // name of the write
                          GENERIC_READ,          
                          FILE_SHARE_READ,         
                          NULL,                   // default security
                          OPEN_EXISTING,            
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                          NULL);
#elif defined(USE_WCHAR_PATH)
        auto fileHandle = ::CreateFileW(m_filename.c_str(),                // name of the write
                          GENERIC_READ,          
                          FILE_SHARE_READ,         
                          NULL,                   // default security
                          OPEN_EXISTING,             
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                          NULL);

#else
#error "undefined path type! USE_CHAR_PATH for std::string, USE_WCHAR_PATH for std::wstring"
#endif
        assert(fileHandle != INVALID_HANDLE_VALUE);
        DWORD sizeHigh = 0;
        DWORD sizeLow = ::GetFileSize(fileHandle, &sizeHigh);
        m_fileSize = sizeLow;
        if (sizeHigh != 0) {
            m_fileSize += static_cast<uint64_t>(sizeHigh) << (sizeof(sizeHigh) * 8);
        }
        m_file.assign(fileHandle);
    }

    void SetupTimerAndStart()
    {
        m_timer.expires_after(std::chrono::milliseconds{m_reportInterval});
        m_timer.async_wait(std::bind(&UploadFileRequest::TimerHandler, this, std::placeholders::_1));
    }
    
    void ReadFromFile()
    {
        assert(WritableSpaceSize() !=  0);
        m_file.async_read_some_at(m_bytesHasBeenReadFromFile,
                    asio::buffer(m_writePos, WritableSpaceSize()),
                    std::bind(&UploadFileRequest::ReadFileHandler, 
                             this, std::placeholders::_1, std::placeholders::_2));
        m_lastReadFileFinished = false;
    }
    void StartReadFromFile()
    {
        ReadFromFile();
    }
    
    void InitWhenTransBegin()
    {
        ReportUploadStart(m_filenameOnServer, m_filename);
        SetupTimerAndStart();
        OpenFile();
        AllocateBuffer();
        StartReadFromFile();
    }
    
    void ReportProgress()
    {
        ReportUploadProgress(m_filenameOnServer, m_filename, m_bytesHasBeenSend, m_fileSize);
    }
    
    void ReleaseOnError(const std::string &errmsg)
    {
        ReportUploadError(m_filenameOnServer, m_filename, errmsg);
        m_file.close();
        m_socket.close();
        if (m_acceptor) {
            m_acceptor->close();
        }
    }

    size_t WritableSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_bufEndPos - m_writePos);
    }

    size_t ReadableSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_writePos - m_readPos);
    }

    size_t RecycleSpaceSize()
    {
        assert(m_readPos <= m_writePos);
        return static_cast<size_t>(m_readPos - m_buf.get());
    }

    void OnComplete()
    {
        // 请求已经完成
        ReportUploadFinish(m_filenameOnServer, m_filename);
        m_complete = true;
    } 
private:
    // 上传到服务器的文件名, 
    // 要求: (1) 路径全名; (2) 文件已存在
    FTPClientPathType   m_filename;
    // 服务器上的文件名(全路径或者相对路径)
    std::string m_filenameOnServer;
    // 主动模式下为服务器ip, 被动模式下无意义
    std::string m_ip; 
    // 主动模式下为服务器port, 被动模式下为监听端口
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket    m_socket;
    // 文件上传是否已完成
    bool        m_complete = false;
    // 用于发送数据的缓冲区
    // 从文件读取的数据暂存于缓冲区, 然后将缓冲区的数据发送到服务器
    std::unique_ptr<char[]> m_buf;
    std::size_t   m_bufSize;
    // 从文件向缓冲区写数据时, 缓冲区空闲容量的最小值
    static constexpr size_t kMinFreeWriteSapceSize = 256;
    // 将读取的文件数据写入缓冲区的起始地址
    char  *m_writePos = nullptr;
    // 网络发送数据的起始地址
    char  *m_readPos = nullptr;
    // 缓冲区结束位置
    char  *m_bufEndPos = nullptr;
    // 文件读取数据策略
    random_access_handle m_file;
    // 上一次异步读文件是否完成
    bool  m_lastReadFileFinished = true;
    // 上一次网络发送数据完成
    bool  m_lastWriteNetworkFinished = true;
    // 由于tcp缓存不足, 阻塞文件数据的读取
    bool  m_blockFileRead = false;
    // 文件是否已读完
    bool  m_fileReadFinished = false;
    // 被动连接模式下, 表示是否已和服务器建立了连接
    bool  m_accepted = false;
    // 已经读取的文件字节数
    uint64_t m_bytesHasBeenReadFromFile = 0;
    // 已经发送的数据的字节数
    uint64_t m_bytesHasBeenSend = 0;
    // 最近一次异步写网络数据请求的字节数
    size_t   m_lastAsycWriteInBytes = 0;
    // 上传的文件大小
    uint64_t m_fileSize = 0;
    // 通知进度间隔
    size_t  m_reportInterval;
    asio::steady_timer m_timer;
};

// 实现思路:
//   将网络读取的数据写入缓冲区, 如果缓冲区空间不足, 则重新分配缓冲区.
template <class NotifyMsgPolicy>
class DownloadDirContentRequest : public DataTransRequest, public NotifyMsgPolicy
{
public:    
    // 主动连接服务器
    DownloadDirContentRequest(
                        IOContextPointer io, 
                        const std::string &dirPath, 
                        const std::string &ip,  // 服务器ip
                        uint16_t port, // 服务器端口
                        size_t bufSize = 1024) // 缓冲区起始地址
        :m_io(io),
        m_socket(*io),
        m_dirPath(dirPath),
        m_ip(ip),
        m_port(port),
        m_passiveMode(false),
        m_bufSize(bufSize)
    {}
                        
    // 监听端口, 等待服务器连接
    DownloadDirContentRequest(
                        IOContextPointer io,
                        const std::string &dirPath, 
                        uint16_t port,
                        size_t bufSize = 1024)
        :m_io(io),
        m_socket(*io),
        m_dirPath(dirPath),
        m_port(port),
        m_bufSize(bufSize),
        m_passiveMode(true)
    {}
                        
public:
    void Start() override
    {
        LOG(INFO) << "DownloadDirContentRequest::Start()";
        if (m_passiveMode) {
            m_acceptor = std::move(std::make_unique<tcp::acceptor>(*m_io));
            m_acceptor->open(tcp::v4());
            m_acceptor->bind(tcp::endpoint(tcp::v4(), m_port));
            m_acceptor->async_accept(m_socket, std::bind(&DownloadDirContentRequest::AcceptHandler,
                                        this, std::placeholders::_1));
            return;
        }
        m_socket.async_connect(tcp::endpoint(address::from_string(m_ip), m_port), 
                                 std::bind(&DownloadDirContentRequest::ConnectHandler, 
                                           this, std::placeholders::_1));

    }
    bool Complete() override
    {
        return m_complete;
    }
    void Destroy() override
    {
        if (m_socket.is_open()) {
            m_socket.close();
        }
        if (m_acceptor && m_acceptor->is_open()) {
            m_acceptor->close();
        }
        delete this;
    }
    
public:
    void ConnectHandler(const asio::error_code &error)
    {
        if (!error) {
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "UploadFileRequest::ConnectHandler() failed: server(" << m_ip << ":" <<m_port
                       << "), " <<  error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void AcceptHandler(const asio::error_code& error)
    {
        if (!error) {
            // 不允许在监听模式下建立多个tcp连接
            m_acceptor->close();
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "UploadFileRequest::AcceptHandler() failed: " << "host port(" << m_port << ")," 
                       << error.message() << ", error code " << error.value();
            ReleaseOnError(error.message());
        }
    }
    void ReadHandler(const asio::error_code &error, size_t bytesRead)
    {
        if (!error) {
            m_writePos += bytesRead;
            if (bytesRead == 0) {
                OnComplete();
                return;
            }
            if (WritableSpaceSize() < kMinWritableSpace) {
                ResizeBuffer();
            }
            ReadFromNetwork();
        } else {
            if (error ==  asio::error_code{asio::error::eof, asio::error::get_misc_category()}) {
                OnComplete();
            } else {
                LOG(ERROR) << "UploadFileRequest::ReadHandler() failed: " << error.message() << ", error code"
                           << error.value();
                ReleaseOnError(error.message());
            }
        }
    }
protected:  
    ~DownloadDirContentRequest() = default;
    void AllocateBuffer()
    {
        if (!m_buf) {
            m_buf = std::move(std::unique_ptr<char []>(new char[m_bufSize]));
            m_writePos = &m_buf[0];
            m_bufEndPos = &m_buf[0] + m_bufSize;
        }    
    }
    void ResizeBuffer()
    {
        try {
            auto newBufSize = m_bufSize + m_sizeDelta;
            auto newBuf = std::move(std::make_unique<char[]>(newBufSize));   
            std::memcpy(newBuf.get(), m_buf.get(), m_bufSize);
            m_writePos = newBuf.get() + static_cast<size_t>(m_writePos - m_buf.get());
            m_bufEndPos = newBuf.get() + newBufSize;
            m_bufSize = newBufSize;
            m_buf = std::move(newBuf);
        } catch(const std::bad_alloc &ex) {
            LOG(FATAL) << "DownloadDirContentRequest::ResizeBuffer(): std::bad_alloc exception, " << ex.what();
        }
    }
    void ReadFromNetwork()
    {
        assert(WritableSpaceSize() !=  0);
        m_socket.async_read_some(asio::buffer(m_writePos, WritableSpaceSize()), 
                        std::bind(&DownloadDirContentRequest::ReadHandler, 
                             this, std::placeholders::_1, std::placeholders::_2));
    }
    void StartReadFromNetwork()
    {
        ReadFromNetwork();
    }
    void InitWhenTransBegin()
    {
        AllocateBuffer();
        StartReadFromNetwork();
    }
    void ReleaseOnError(const std::string &errmsg)
    {
        ReportGetDirContentError(errmsg);
        m_socket.close();
        if (m_acceptor) {
            m_acceptor->close();
        }
    }

    size_t WritableSpaceSize()
    {
        assert(m_bufEndPos >= m_writePos);
        return static_cast<size_t>(m_bufEndPos - m_writePos);
    }

    size_t ReadableSpaceSize()
    {
        assert(m_buf.get() <= m_writePos);
        return static_cast<size_t>(m_writePos - m_buf.get());
    }
    void OnComplete()
    {
        ReportDirContent(m_dirPath, std::move(m_buf), ReadableSpaceSize());
        // 请求已经完成
        m_complete = true;
    } 
private:
    // 目录内容所属的路径, 为路径全名
    std::string m_dirPath;
    // 主动模式下为服务器ip, 被动模式下无意义
    std::string m_ip; 
    // 主动模式下为服务器port, 被动模式下为监听端口
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket    m_socket;
    // 目录内容传输是否完成
    bool        m_complete = false;
    // 每次从网络接收数据要求的最小缓冲区空间
    static constexpr size_t kMinWritableSpace = 256;
    // 缓冲区
    std::unique_ptr<char[]>  m_buf;
    // 缓冲区大小
    size_t  m_bufSize;
    // 网络接收数据写入缓冲区的起始位置
    char  *m_writePos = nullptr;
    // 缓冲区的结束位置[m_buf.get(), m_bufEndPos)
    char  *m_bufEndPos = nullptr;
    // 被动连接模式下, 表示是否已和服务器建立了连接
    // 如果有新的连接到来, 则拒绝新的连接
    bool  m_accepted = false;
    // 每次buffer空间不够的时候, 增加m_sizeDelta容量
    size_t  m_sizeDelta = 1024;
};

}// namespace ftpclient

#endif // _DATA_TRANS_RELATED_REQUEST_H
