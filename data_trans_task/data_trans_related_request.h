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

// �����ļ���ʵ��˼·
// -------------------
//    �����������������д�ļ�����, ÿ�����������ݵ���ʱ,
// �����첽д�ļ�����������д���ļ�. ��ʵ����, ���õ���������
// ͬʱ���ж�д, �������ݱ���θ���. 
//    �Ƚ����ѵĵط���, ���ڶ��������ݺ�д���ݵ��ļ�
// �����첽��, ��ÿ��д�뻺����, д��ʼ��ַm_writePos������ǰ����,
// �⽫��������m_writePos���ڻ���������λ��. ��m_writePos�����βʱ
// �����°����������Ƶ�������ͷ��. ����첽����δ���, ��ô������
// ���ݽ����ܱ��ƶ�.
//    Ϊ�˽���������, �ڻ������ռ䲻��ʱ, ������������ݲ���. �����Ļ�
// �Ͳ���ʵ�������ȡ�����ļ�д����. ��������m_blockNetworkRead��־, 
// ���첽д�ļ����ʱ, �����������ݱ�����, ��ʱ�����°��Ż������ռ�, 
// Ȼ��ʼ��������ݲ���.
//
// �ļ�����
// --------
// �ļ�������س��ִ���, ���Լ򵥵Ľ����������ļ�ɾ����. ����һ��ʼ������Ϊ
// ��ʱ�ļ�, �����ʱ�����ΪĿ���ļ�, ������س��ִ���, �򱣴�Ϊ��ʱ�ļ�.
// FileSavePolicyҪ��Ľӿ�Ϊ:
//      HANDLE OpenFile(const FTPClientPathType &);
//      SaveFileOnSuccess();
//      SaveFileOnFailure();
// ע��: FileSavePolicy���ܹ���HANDLE, ����Ĺ�������"�ͻ�"���.
//
// ��Ϣ֪ͨ
// --------
// ���ض��ڵ�����Ҫ��"�û���"֪ͨ��������Ϣ. ����ʵ�ֶ�����Ϣ֪ͨ����.
// MsgNotifyҪ��Ľӿ�Ϊ:
//  ReportEvent(const Notify &)
// ��չ֪ͨ:
// --------
// �û�����ѡ��֪ͨ���ؽ�չ����֪ͨ���ؽ�չ
// ReportProgressPolicy<ReportedObject>Ҫ��Ľӿ�Ϊ:
//   ReportProgressPolicy(io_context &);
//   Start(const ReportedObject &, size_t duration); // durationΪ����ֵ
//   Stop()
// ��ReportedObject��Ҫ��Ľӿ�Ϊ:
//   void Report();
//
template <class FileSavePolicy, class MsgNotify>
class DownloadFileRequest : public DataTransRequest, 
                                 public FileSavePolicy, 
                                 public MsgNotify
{
public:
    // �������ӷ�����
    DownloadFileRequest(
                        IOContextPointer io, 
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer, 
                        const std::string &ip,  // ������ip
                        uint16_t port, // �������˿�
                        uint64_t  fileSize = 0,  // �ļ���С
                        size_t  interval = 500, // ���ȱ���ʱ����, ��ms����
                        uint16_t bufSize = 32768 ) // ���ջ�������С(32KiBĬ��)
        
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
                        
    // �����˿�, �ȴ�����������
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
            // �û��������ļ���δ�򿪻����ļ�������δ��ɵ�������ֶ�
            // ��ֹ�߳�
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
            // �����������tcp����
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
                // ���ݽ������
                m_socket.close();
                // ��ʱm_complete������������, ��Ҫ�ȴ�
                // �ļ��첽д����ɲ�����
                m_dataTransComplete = true; 
                return;
            }
            if (m_lastWriteFileFinished) {
                // ��һ���첽д�ļ��Ѿ����, ���Կ�ʼ����д�ļ�
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
                // �ļ�д�����
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
            // �ļ�����û�б�д��, ���ش���
            LOG(FATAL) << "DownloadFileRequest::RecycleBuffer(): after move buffer, still no free buffer space to use!";
        }
    }
    void OpenFile()
    {
        // FileSavePolicy�������ļ����
        // �������ļ����Ϳ���ʵ����ʱ�ļ�����ʱ�ļ���
        // ���ﷵ�صľ�����û��������
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
            // �����ļ�ǰ��֪���ļ���С�Ļ�, Ĭ������������
            SaveFileOnSuccess();
        }
        // �����Ѿ����
        m_complete = true;
    }
private:
    // ���浽���ص��ļ���, 
    // Ҫ��: (1) ·��ȫ��; (2) �ļ�������
    FTPClientPathType       m_filename;
    // �����������ص��ļ���(ȫ·���������·��)
    std::string m_filenameOnServer;
    // ����ģʽ��Ϊ������ip, ����ģʽ��������
    std::string m_ip; 
    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    // �ļ������Ƿ����
    bool        m_complete = false;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket     m_socket;
    random_access_handle m_file;
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
    // ��һ���첽д�ļ��Ƿ����?
    bool             m_lastWriteFileFinished = true;
    // ��������д���ݻ���������, ��ʱ�����������ȡ����
    bool             m_blockNetworkRead = false;
    // �������ݴ����Ƿ����(eof)
    bool             m_dataTransComplete = false;
    // �ڿͻ��˱�������ģʽ��, ��ʾ�Ƿ�
    // �Ѿ��ͷ���������������(������ͷ���������һ������)
    bool             m_accepted = false;
    // �Ѿ�д���ļ����ֽ���
    uint64_t         m_bytesHasBeenWriteToFile = 0;
    // �ļ���С
    // ����ǿ�ѡ��Ļ�, m_fileSizeΪ0
    uint64_t         m_fileSize;
    // ���ȱ�����, ����ms����
    size_t           m_reportInterval;
    // 
    asio::steady_timer m_timer;
    
};

// UploadFileRequest��DownloadFileRequestʵ������. 
// ������ļ���ȡ�����൱��DownloadFileRequest�������ļ���ȡ����,
// �����д�������ݲ����൱��DownloadFileRequest��д�ļ�����.
template <class MsgNotifyPolicy>
class UploadFileRequest : public DataTransRequest, public MsgNotifyPolicy
{
public:
    // �������ӷ�����
    UploadFileRequest(
                        IOContextPointer io, 
                        const FTPClientPathType &filename, 
                        const std::string &filenameOnServer,
                        const std::string &ip,  // ������ip
                        uint16_t port, // �������˿�
                        size_t  interval = 500, // ÿ500ms֪ͨһ�ν���
                        uint16_t bufSize = 32768) // ���ͻ�������С(32KiBĬ��)
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
       
    // �����˿�, �ȴ�����������
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
            // �û��������ļ���δ�򿪻����ļ�������δ��ɵ�������ֶ�
            // ��ֹ�߳�
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
                // �ļ��Ѷ���
                m_file.close();
                // ��ʱm_complete������������, ��Ҫ�ȴ�
                // ���紫����ɲ�����
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
                // �ļ�д�����
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
            // �ļ�����û�б�д��, ���ش���
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
        // �����Ѿ����
        ReportUploadFinish(m_filenameOnServer, m_filename);
        m_complete = true;
    } 
private:
    // �ϴ������������ļ���, 
    // Ҫ��: (1) ·��ȫ��; (2) �ļ��Ѵ���
    FTPClientPathType   m_filename;
    // �������ϵ��ļ���(ȫ·���������·��)
    std::string m_filenameOnServer;
    // ����ģʽ��Ϊ������ip, ����ģʽ��������
    std::string m_ip; 
    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket    m_socket;
    // �ļ��ϴ��Ƿ������
    bool        m_complete = false;
    // ���ڷ������ݵĻ�����
    // ���ļ���ȡ�������ݴ��ڻ�����, Ȼ�󽫻����������ݷ��͵�������
    std::unique_ptr<char[]> m_buf;
    std::size_t   m_bufSize;
    // ���ļ��򻺳���д����ʱ, ������������������Сֵ
    static constexpr size_t kMinFreeWriteSapceSize = 256;
    // ����ȡ���ļ�����д�뻺��������ʼ��ַ
    char  *m_writePos = nullptr;
    // ���緢�����ݵ���ʼ��ַ
    char  *m_readPos = nullptr;
    // ����������λ��
    char  *m_bufEndPos = nullptr;
    // �ļ���ȡ���ݲ���
    random_access_handle m_file;
    // ��һ���첽���ļ��Ƿ����
    bool  m_lastReadFileFinished = true;
    // ��һ�����緢���������
    bool  m_lastWriteNetworkFinished = true;
    // ����tcp���治��, �����ļ����ݵĶ�ȡ
    bool  m_blockFileRead = false;
    // �ļ��Ƿ��Ѷ���
    bool  m_fileReadFinished = false;
    // ��������ģʽ��, ��ʾ�Ƿ��Ѻͷ���������������
    bool  m_accepted = false;
    // �Ѿ���ȡ���ļ��ֽ���
    uint64_t m_bytesHasBeenReadFromFile = 0;
    // �Ѿ����͵����ݵ��ֽ���
    uint64_t m_bytesHasBeenSend = 0;
    // ���һ���첽д��������������ֽ���
    size_t   m_lastAsycWriteInBytes = 0;
    // �ϴ����ļ���С
    uint64_t m_fileSize = 0;
    // ֪ͨ���ȼ��
    size_t  m_reportInterval;
    asio::steady_timer m_timer;
};

// ʵ��˼·:
//   �������ȡ������д�뻺����, ����������ռ䲻��, �����·��仺����.
template <class NotifyMsgPolicy>
class DownloadDirContentRequest : public DataTransRequest, public NotifyMsgPolicy
{
public:    
    // �������ӷ�����
    DownloadDirContentRequest(
                        IOContextPointer io, 
                        const std::string &dirPath, 
                        const std::string &ip,  // ������ip
                        uint16_t port, // �������˿�
                        size_t bufSize = 1024) // ��������ʼ��ַ
        :m_io(io),
        m_socket(*io),
        m_dirPath(dirPath),
        m_ip(ip),
        m_port(port),
        m_passiveMode(false),
        m_bufSize(bufSize)
    {}
                        
    // �����˿�, �ȴ�����������
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
            // �������ڼ���ģʽ�½������tcp����
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
        // �����Ѿ����
        m_complete = true;
    } 
private:
    // Ŀ¼����������·��, Ϊ·��ȫ��
    std::string m_dirPath;
    // ����ģʽ��Ϊ������ip, ����ģʽ��������
    std::string m_ip; 
    // ����ģʽ��Ϊ������port, ����ģʽ��Ϊ�����˿�
    uint16_t    m_port;
    bool        m_passiveMode;
    IOContextPointer m_io;
    std::unique_ptr<tcp::acceptor> m_acceptor;
    tcp::socket    m_socket;
    // Ŀ¼���ݴ����Ƿ����
    bool        m_complete = false;
    // ÿ�δ������������Ҫ�����С�������ռ�
    static constexpr size_t kMinWritableSpace = 256;
    // ������
    std::unique_ptr<char[]>  m_buf;
    // ��������С
    size_t  m_bufSize;
    // �����������д�뻺��������ʼλ��
    char  *m_writePos = nullptr;
    // �������Ľ���λ��[m_buf.get(), m_bufEndPos)
    char  *m_bufEndPos = nullptr;
    // ��������ģʽ��, ��ʾ�Ƿ��Ѻͷ���������������
    // ������µ����ӵ���, ��ܾ��µ�����
    bool  m_accepted = false;
    // ÿ��buffer�ռ䲻����ʱ��, ����m_sizeDelta����
    size_t  m_sizeDelta = 1024;
};

}// namespace ftpclient

#endif // _DATA_TRANS_RELATED_REQUEST_H
