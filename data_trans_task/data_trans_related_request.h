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
    // �첽��ʽд���ݵ��ļ�
    template <typename WriteHandler, class ConstBufferSequence>
    void AsyncWrite(uint64_t offset, const ConstBufferSequence &, WriteHandler &&handler, bool& actuallyWrite);
    // ȡ����δ��ɵ�async_write����
    void Cancel();
    // д����������, ʵ�ֲ�ͬ���ļ��������
    void CloseOnComplete();
    // �ļ�д����̷����������׼�����ݹ��̳��ִ������ļ����ݲ�����ʱ,
    // ʹ�ô˵���. ����, ��ε���֮Ӧ������
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
    // �첽��ʽд���ݵ��ļ�
    template <typename WriteHandler, class ConstBufferSequence>
    void AsyncWrite(const ConstBufferSequence &, WriteHandler &&handler, bool& actuallyWrite);
    // ȡ����δ��ɵ�async_write����
    void Cancel();
    // д����������, ʵ�ֲ�ͬ���ļ��������
    void CloseOnComplete();
    // �ļ�д����̷����������׼�����ݹ��̳��ִ������ļ����ݲ�����ʱ,
    // ʹ�ô˵���. ����, ��ε���֮Ӧ������
    void CloseOnFailure();
};

template <class Path, template <class Path > class FileSavePolicy>
class DownloadFileRequest : public DataTransRequest
{
public:
    // �������ӷ�����
    DownloadFileRequest(
                        IOContextPointer io, 
                        const Path &filename, 
                        const std::string &ip,  // ������ip
                        uint16_t port, // �������˿�
                        uint16_t bufSize = 32768 ) // ���ջ�������С(32KiBĬ��)
        :m_io(io),
        m_filename(filename),
        m_ip(ip),
        m_port(port),
        m_socket(*m_io),
        m_passiveMode(false),
        m_fileSavePolicy(m_io),
        m_bufSize(bufSize)
    {}
                        
    // �����˿�, �ȴ�����������
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
            // �û��������ļ���δ�򿪻����ļ�������δ��ɵ�������ֶ�
            // ��ֹ�߳�
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
                // ����֮ǰ���Ѿ�����
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
                // ���ݽ������
                m_socket.close();
                // ��ʱm_complete������������, ��Ҫ�ȴ�
                // �ļ��첽д����ɲ�����
                m_dataTransComplete = true; 
                return;
            }
            m_writePos += bytesRead;
            if (m_lastWriteFileFinished) {
                if (ReadableSpaceSize() != 0) {
                    // ��һ���첽д�ļ��Ѿ����, ���Կ�ʼ����д�ļ�
                    bool callAsyncWrite = false;
                    m_fileSavePolicy.AsyncWrite(m_bytesHasBeenWriteToFile, asio::buffer(m_readPos, ReadableSpaceSize()), 
                           std::bind(&DownloadFileRequest::WriteFileHandler, this, std::placeholders::_1, 
                            std::placeholders::_2), callAsyncWrite);
                    m_lastWriteFileFinished = callAsyncWrite ? false : true;
                }
                if ((m_bufEndPos - m_writePos) < kWriteMinWindowSize) {
                    // ����������д�ռ䲻����Ļ���Ų���ռ�
                    if (!m_lastWriteFileFinished) {
                        // �첽д�ļ�δ���, �����ƶ�����������
                        m_blockNetworkRead = true;
                        return;
                    }
                    // ����������������д�ռ䲻��, ��֮ǰ���첽���������, 
                    // ��˿����ƶ�����������, ���OS����������˵û��Ӱ��
                    RecycleBuffer();
                }   
                ReadFromNetwork();
                return;
            } else if (!m_lastWriteFileFinished && ( WritableSpaceSize() < kWriteMinWindowSize)) {
                // ����������������д�Ŀռ䲻��, Ȼ�����һ���첽д��δ���, ֻ��
                // ��ͣ��ȡ��������, �ȴ��ļ�д�����, ������Զ������������ݴ����ٶ�
                // �ı�־��WriteFileHandler()ʹ��, ������������������
                m_blockNetworkRead = true;
                return ;
            } else {
                // ���һ���첽д�ļ�δ���, ��Ȼ����д�ļ�, �����绺�����ǳ����
                // ���Լ����������ȡ����
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
                // �ļ�д�����
                m_fileSavePolicy.CloseOnComplete();
                // �����Ѿ����
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
            // �ļ�����û�б�д��, ���ش���
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
    // ���浽���ص��ļ���, 
    // Ҫ��: (1) ·��ȫ��; (2) �ļ�������
    Path       m_filename;
    // ����ģʽ��Ϊ������ip, ����ģʽ��������
    std::string m_ip; 
    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    // �ļ������Ƿ����
    bool        m_complete;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket     m_socket;
    // ���ݻ�����
    // ������д���ݵĻ�������С���ڴ�С
    // �����������������С����, ����Ҫ��m_writePos��m_readPos
    // ���½��й���
    static constexpr size_t kWriteMinWindowSize = 256;
    std::unique_ptr<char[]> m_buf;
    std::size_t    m_bufSize;
    // ������յ������򻺳���д�����ʼλ��
    char    *m_writePos = nullptr;
    // д�ļ�ʱ����ʼλ��
    char     *m_readPos  = nullptr;
    // ����������λ��[0, m_bufEndPos)
    char     *m_bufEndPos = nullptr;    
    // �ļ��������
    FileSavePolicy<Path> m_fileSavePolicy;
    // ��һ���첽д�ļ��Ƿ����?
    bool             m_lastWriteFileFinished = true;
    // ��������д���ݻ���������, ��ʱ�����������ȡ����
    bool             m_blockNetworkRead = false;
    // �������ݴ����Ƿ����
    bool             m_dataTransComplete = false;
    // �ڿͻ��˱�������ģʽ��, ��ʾ�Ƿ�
    // �Ѿ��ͷ���������������(������ͷ���������һ������)
    bool             m_accepted = false;
    // �Ѿ�д���ļ����ֽ���
    uint64_t         m_bytesHasBeenWriteToFile = 0;
};

//
//template <typename Path>
//class UploadFileRequest : public DataTransRequest
//{
//public:
//    // �������ӷ�����
//    UploadFileRequest(
//                        IOContextPointer io, 
//                        const Path &filename, 
//                        const std::string &ip,  // ������ip
//                        uint16_t port) // �������˿�
//        :m_io(io),
//        m_filename(filename),
//        m_ip(ip),
//        m_port(port),
//        m_passiveMode(false)
//    {}
//       
//    // �����˿�, �ȴ�����������
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
//    // �ϴ������������ļ���, 
//    // Ҫ��: (1) ·��ȫ��; (2) �ļ��Ѵ���
//    Path   m_filename;
//    // ����ģʽ��Ϊ������ip, ����ģʽ��������
//    std::string m_ip; 
//    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
//    uint16_t    m_port;
//    bool        m_passiveMode;
//    IOContextPointer m_io;
//};
//
//class DownloadDirContentRequest : public DataTransRequest
//{
//public:    
//    // �������ӷ�����
//    DownloadDirContentRequest(
//                        IOContextPointer io, 
//                        const std::string &dirPath, 
//                        const std::string &ip,  // ������ip
//                        uint16_t port) // �������˿�
//        :m_io(io),
//        m_dirPath(dirPath),
//        m_ip(ip),
//        m_port(port),
//        m_passiveMode(false)
//    {}
//                        
//    // �����˿�, �ȴ�����������
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
//    // Ŀ¼����������·��, Ϊ·��ȫ��
//    std::string m_dirPath;
//    // ����ģʽ��Ϊ������ip, ����ģʽ��������
//    std::string m_ip; 
//    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
//    uint16_t    m_port;
//    bool        m_passiveMode;
//    IOContextPointer m_io;
//};



}// namespace ftpclient

#endif // _DATA_TRANS_RELATED_REQUEST_H
