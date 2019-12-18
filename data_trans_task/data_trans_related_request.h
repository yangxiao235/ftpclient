#ifndef _DATA_TRANS_RELATED_REQUEST_H
#define _DATA_TRANS_RELATED_REQUEST_H

#include "ftpclient/config.h"
#include "ftpclient/common_types.h"
#include "data_trans_request.h"
#include "data_trans_request_pending_list.h"
#include <asio.hpp>
#include <glog/logging.h>

namespace ftpclient {

using asio::windows::random_access_handle;

template <class Path>
class SimpleFileSavePolicy;

class SimpleFileSavePolicyImpl;

template <>
class SimpleFileSavePolicy<std::string>
{
public:
    SimpleFileSavePolicy(IOContextPointer io);
    ~SimpleFileSavePolicy();
    void OpenFile(const std::string &file, std::error_code &ec);
    // 异步方式写数据到文件
    template <typename WriteHandler, class ConstBufferSequence>
    void AsyncWrite(uint64_t offset, const ConstBufferSequence &, WriteHandler &&handler, bool& actuallyWrite);
    // 取消尚未完成的async_write任务
    void Cancel();
    // 写入已完成完成, 实现不同的文件保存策略
    void CloseOnComplete();
    // 文件写入过程发生错误或者准备数据过程出现错误导致文件数据不完整时,
    // 使用此调用. 另外, 多次调用之应该无伤
    void CloseOnFailure();
private:
    random_access_handle   m_handle;
};

template <typename WriteHandler, class BufferSequence>
void SimpleFileSavePolicy<std::string>::AsyncWrite(uint64_t offset, const BufferSequence &buf, WriteHandler &&handler, bool& callAsycWrite)
{
    callAsycWrite = false;
    if (buf.size() >= 4096) {
        m_handle.async_write_some_at(offset, buf, handler);
        callAsycWrite = true;
    }
}


template <class Path>
class TemporaryFileSavePolicy
{
public:
    TemporaryFileSavePolicy(IOContextPointer io);
    void OpenFile(const Path &file, std::error_code &ec);
    // 异步方式写数据到文件
    template <typename WriteHandler, class ConstBufferSequence>
    void AsyncWrite(const ConstBufferSequence &, WriteHandler &&handler, bool& actuallyWrite);
    // 取消尚未完成的async_write任务
    void Cancel();
    // 写入已完成完成, 实现不同的文件保存策略
    void CloseOnComplete();
    // 文件写入过程发生错误或者准备数据过程出现错误导致文件数据不完整时,
    // 使用此调用. 另外, 多次调用之应该无伤
    void CloseOnFailure();
};

template <class Path, template <class Path > class FileSavePolicy>
class DownloadFileRequest : public DataTransRequest
{
public:
    // 主动连接服务器
    DownloadFileRequest(
                        IOContextPointer io, 
                        const Path &filename, 
                        const std::string &ip,  // 服务器ip
                        uint16_t port, // 服务器端口
                        uint16_t bufSize = 32768 ) // 接收缓冲区大小(32KiB默认)
        :m_io(io),
        m_filename(filename),
        m_ip(ip),
        m_port(port),
        m_socket(*m_io),
        m_passiveMode(false),
        m_fileSavePolicy(m_io),
        m_bufSize(bufSize)
    {}
                        
    // 监听端口, 等待服务器连接
    DownloadFileRequest(
                        IOContextPointer io,
                        const Path &filename, 
                        uint16_t port,
                        size_t bufSize = 32768 )
        :m_io(io),
        m_filename(filename),
        m_port(port),
        m_socket(*m_io),
        m_fileSavePolicy(m_io),
        m_bufSize(bufSize),
        m_passiveMode(true)
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
            m_fileSavePolicy.CloseOnFailure();
        } 
        if (m_socket.is_open()) {
            m_socket.close();
        }
        if (m_acceptor && m_acceptor->is_open()) {
            m_acceptor->close();
        }
        delete this;
    }
protected:
    ~DownloadFileRequest() = default;
    void ConnectHandler(const asio::error_code &error)
    {
        if (!error) {
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "DownloadFileRequest::ConnectHandler() failed: server(" << m_ip << ":" <<m_port
                       << "), " <<  error.message() << ", error code " << error.value();
            ReleaseOnError();
        }
    }
    void AcceptHandler(const asio::error_code& error)
    {
        if (!error) {
            if (!m_accepted) {
                m_accepted = true;
            }
            if (m_accepted) {
                // 连接之前就已经建立
                LOG(ERROR) << "Multiple connection in accept mode is not allowed!";
                ReleaseOnError();
                return;
            }
            InitWhenTransBegin();
        } else {
            LOG(ERROR) << "DownloadFileRequest::AcceptHandler() failed: " << "host port(" << m_port << ")," 
                       << error.message() << ", error code " << error.value();
            ReleaseOnError();
        }
    }
    void ReadHandler(const asio::error_code &error, size_t bytesRead)
    {
        if (!error) {
            if (bytesRead == 0) {
                // 数据接收完成
                m_socket.close();
                // 此时m_complete不会立即设置, 需要等待
                // 文件异步写入完成才设置
                m_dataTransComplete = true; 
                return;
            }
            m_writePos += bytesRead;
            if (m_lastWriteFileFinished) {
                if (ReadableSpaceSize() != 0) {
                    // 上一次异步写文件已经完成, 可以开始继续写文件
                    bool callAsyncWrite = false;
                    m_fileSavePolicy.AsyncWrite(m_bytesHasBeenWriteToFile, asio::buffer(m_readPos, ReadableSpaceSize()), 
                           std::bind(&DownloadFileRequest::WriteFileHandler, this, std::placeholders::_1, 
                            std::placeholders::_2), callAsyncWrite);
                    m_lastWriteFileFinished = callAsyncWrite ? false : true;
                }
                if ((m_bufEndPos - m_writePos) < kWriteMinWindowSize) {
                    // 供网络数据写空间不充足的话腾挪出空间
                    if (!m_lastWriteFileFinished) {
                        // 异步写文件未完成, 不能移动缓冲区数据
                        m_blockNetworkRead = true;
                        return;
                    }
                    // 缓冲区供网络数据写空间不足, 且之前的异步任务都已完成, 
                    // 因此可以移动缓冲区数据, 这对OS驱动程序来说没有影响
                    RecycleBuffer();
                }   
                ReadFromNetwork();
                return;
            } else if (!m_lastWriteFileFinished && ( WritableSpaceSize() < kWriteMinWindowSize)) {
                // 缓冲区供网络数据写的空间不足, 然而最近一次异步写还未完成, 只好
                // 暂停读取网络数据, 等待文件写入完成, 这可以自动控制网络数据传送速度
                // 改标志由WriteFileHandler()使用, 用来驱动读网络数据
                m_blockNetworkRead = true;
                return ;
            } else {
                // 最近一次异步写文件未完成, 虽然不能写文件, 但网络缓冲区是充足的
                // 可以继续从网络读取数据
                ReadFromNetwork();
            }
        }else {
            if (error ==  asio::error_code{asio::error::eof, asio::error::get_misc_category()}) {
                LOG(INFO) << "TCP connection has been closed";
                m_dataTransComplete = true;
                if (m_lastWriteFileFinished) {
                    m_complete = true;
                }
            } else {
                LOG(ERROR) << "DownloadFileRequest::ReadHandler() failed: " << error.message() << ", error code"
                           << error.value();
            }
            ReleaseOnError();
        }
    }

    void WriteFileHandler( const asio::error_code& error, std::size_t bytesWrite)
    {
        if (!error) {
            m_bytesHasBeenWriteToFile += bytesWrite;
            m_readPos += bytesWrite;
            if (ReadableSpaceSize() && m_dataTransComplete) {
                // 文件写入完成
                m_fileSavePolicy.CloseOnComplete();
                // 请求已经完成
                m_complete = true;
                return;
            }
            m_lastWriteFileFinished = true;
            if (m_blockNetworkRead) {
                m_blockNetworkRead = false;
                if (WritableSpaceSize() < kWriteMinWindowSize) {
                    RecycleBuffer();
                }
                ReadFromNetwork();
            }
        } else {
            LOG(ERROR) << "DownloadFileRequest::WriteHandler() failed: " << error.message() << ", error code "
                       << error.value();
            ReleaseOnError();
        }
    }
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
            ReleaseOnError();
            return;
        }
    }
    void OpenFile()
    {
        std::error_code ec;
        m_fileSavePolicy.OpenFile(m_filename, ec);
        assert(!ec);
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
        OpenFile();
        AllocateBuffer();
        StartReadFromNetwork();
    }
    void ReleaseOnError()
    {
        m_socket.close();
        m_fileSavePolicy.CloseOnFailure();
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
private:
    // 储存到本地的文件名, 
    // 要求: (1) 路径全名; (2) 文件不存在
    Path       m_filename;
    // 主动模式下为服务器ip, 被动模式下无意义
    std::string m_ip; 
    // 主动模式下为服务器port, 被动模式下为监听端口
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    // 文件下载是否完成
    bool        m_complete;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket     m_socket;
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
    // 文件保存策略
    FileSavePolicy<Path> m_fileSavePolicy;
    // 上一次异步写文件是否完成?
    bool             m_lastWriteFileFinished = true;
    // 由于网络写数据缓冲区不足, 暂时阻塞从网络读取数据
    bool             m_blockNetworkRead = false;
    // 网络数据传输是否完成
    bool             m_dataTransComplete = false;
    // 在客户端被动连接模式下, 表示是否
    // 已经和服务器建立了连接(仅允许和服务器建立一个连接)
    bool             m_accepted = false;
    // 已经写入文件的字节数
    uint64_t         m_bytesHasBeenWriteToFile = 0;
};

//
//template <typename Path>
//class UploadFileRequest : public DataTransRequest
//{
//public:
//    // 主动连接服务器
//    UploadFileRequest(
//                        IOContextPointer io, 
//                        const Path &filename, 
//                        const std::string &ip,  // 服务器ip
//                        uint16_t port) // 服务器端口
//        :m_io(io),
//        m_filename(filename),
//        m_ip(ip),
//        m_port(port),
//        m_passiveMode(false)
//    {}
//       
//    // 监听端口, 等待服务器连接
//    UploadFileRequest(
//                        IOContextPointer io,
//                        const Path &filename, 
//                        uint16_t port)
//        :m_io(io),
//        m_filename(filename),
//        m_port(port),
//        m_passiveMode(true)
//    {}
//                        
//public:
//    void Start() override; 
//    bool Complete() override;
//    void Destroy() override;
//private:
//    // 上传到服务器的文件名, 
//    // 要求: (1) 路径全名; (2) 文件已存在
//    Path   m_filename;
//    // 主动模式下为服务器ip, 被动模式下无意义
//    std::string m_ip; 
//    // 主动模式下为服务器port, 被动模式下为监听端口
//    uint16_t    m_port;
//    bool        m_passiveMode;
//    IOContextPointer m_io;
//};
//
//class DownloadDirContentRequest : public DataTransRequest
//{
//public:    
//    // 主动连接服务器
//    DownloadDirContentRequest(
//                        IOContextPointer io, 
//                        const std::string &dirPath, 
//                        const std::string &ip,  // 服务器ip
//                        uint16_t port) // 服务器端口
//        :m_io(io),
//        m_dirPath(dirPath),
//        m_ip(ip),
//        m_port(port),
//        m_passiveMode(false)
//    {}
//                        
//    // 监听端口, 等待服务器连接
//    DownloadDirContentRequest(
//                        IOContextPointer io,
//                        const std::string &dirPath, 
//                        uint16_t port)
//        :m_io(io),
//        m_dirPath(dirPath),
//        m_port(port),
//        m_passiveMode(true)
//    {}
//                        
//public:
//    void Start() override; 
//    bool Complete() override;
//    void Destroy() override;
//private:
//    // 目录内容所属的路径, 为路径全名
//    std::string m_dirPath;
//    // 主动模式下为服务器ip, 被动模式下无意义
//    std::string m_ip; 
//    // 主动模式下为服务器port, 被动模式下为监听端口
//    uint16_t    m_port;
//    bool        m_passiveMode;
//    IOContextPointer m_io;
//};



}// namespace ftpclient

#endif // _DATA_TRANS_RELATED_REQUEST_H
