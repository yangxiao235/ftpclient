#define GLOG_NO_ABBREVIATED_SEVERITIES  
#define GOOGLE_GLOG_DLL_DECL 
#include "network_task.h"
#include "thread_task_error.h"
#include <sstream>
#include <glog/logging.h>

namespace ftpclient {
using asio::io_context;
using asio::buffer;

void FTPClientTask::Init() 
  {
      LOG(INFO) << "FTPClientTask::Init()";
      m_io = std::make_shared<io_context>();
      m_socket = std::make_shared<tcp::socket>(*m_io);
      m_recvBuf = std::move(std::make_unique<std::vector<char>>(512));
      m_recvData = std::move(std::make_unique<std::vector<char>>(1024));
      StartConnect();
  }
  void FTPClientTask::StartConnect()
  {
      LOG(INFO) << "FTPClientTask::StartConnect()" << ", ip: " << m_ip << ", port: " << m_port;
      std::vector<tcp::endpoint> endpoints = {tcp::endpoint{address::from_string(m_ip), m_port}};
      asio::async_connect(*m_socket, endpoints, std::bind(&FTPClientTask::HandleConnect, 
                              this, std::placeholders::_1, std::placeholders::_2));
  }
  
  void FTPClientTask::HandleConnect(const error_code &ec, const tcp::endpoint &endpoint)
  {
      LOG(INFO) << "FTPClientTask::HandleConnect()";
      if (!ec) {
          m_connected = true;
          Notify    msg(MsgType::NETWORK_CONNECT);
          m_notifyQueue.Enqueue(msg);
          m_socket->async_read_some(buffer(m_recvBuf->data(), m_recvBuf->size()),
                          std::bind(&FTPClientTask::HandleRead, this,
                          std::placeholders::_1, std::placeholders::_2));
      } else {
          Notify msg(MsgType::NETWORK_ERROR);
          m_notifyQueue.Enqueue(msg);
          LOG(ERROR) << "Network error:" << ec.message() << ", error code: " << ec.value();
          m_socket->close();
          throw TaskRunFailed("Network connect error");
      }
  }
  
  void FTPClientTask::HandleRead(const error_code &error, size_t bytesRead)
  {
      LOG(INFO) << "FTPClientTask::HandleRead()";
      if (!error) {
          LOG(INFO) << std::string(m_recvBuf->data(), bytesRead);
          // 将本次读到的数据追加到m_recvData
          m_recvBufSize = bytesRead;
          std::memcpy(m_recvData->data() + m_recvDataSize, m_recvBuf->data(), m_recvBufSize);
          m_recvDataSize += m_recvBufSize;
          // 解析ftp回复
          Reply  reply;
          const char *end;
          if (ParseReply(m_recvData->data(), m_recvDataSize, end, reply)) {
              // 解析完成, 将剩余的数据复制到m_recvData开头
              std::memcpy(m_recvData->data(), end, m_recvData->data() + m_recvDataSize - end);
              m_recvDataSize -= (end - m_recvData->data());
              // 通知FTP_REPLY事件
              Notify  msg(MsgType::FTP_REPLY);
              msg.reply = reply;
              m_notifyQueue.Enqueue(msg);
          } else if (m_recvDataSize >= 512) {
              // ftp回复格式错误
              LOG(FATAL) << "FTPClientTask::HandleRea(): m_recvDataSize >= 512, Invalid FTP reply.";
          }
          m_socket->async_read_some(buffer(m_recvBuf->data(), m_recvBuf->size()),
                          std::bind(&FTPClientTask::HandleRead, this,
                          std::placeholders::_1, std::placeholders::_2));
          
      } else {
          // 网络连接错误
          m_connected = false;
          Notify msg(MsgType::NETWORK_ERROR);
          m_notifyQueue.Enqueue(msg);
          LOG(ERROR) << "Network error:" << error.message() << ", error code: " << error.value();
          m_socket->close();
          throw TaskRunFailed("Network Read error");
      }
  }

  void FTPClientTask::HandleWrite(const error_code &error, size_t bytesWrite)
  {
      LOG(INFO) << "FTPClientTask::HandleWrite()";
      if (!error) {
          if (bytesWrite != m_currentCmd.size()) {
              // 记录该错误
              LOG(WARNING) << "FTPClientTask::HandleWrite(): bytesWrite != m_currentCmd->data.size()";
          }
      } else {
          // 处理网路错误
          m_connected = false;
          Notify msg(MsgType::NETWORK_ERROR);
          m_notifyQueue.Enqueue(msg);
          LOG(ERROR) << "Network error:" << error.message() << ", error code: " << error.value();
          m_socket->close(); 
          throw TaskRunFailed("Network write error");
      }
  }
  
  void FTPClientTask::Run()
  {
      LOG(INFO) << "FTPClientTask::Run()";
      if (m_cmdQueue.IsEmpty() || !m_connected) {
          goto __RunIOContext ;
      }
      m_currentCmd = m_cmdQueue.Front();
      m_cmdQueue.Dequeue();
      if (!m_currentCmd.empty()) {
          asio::async_write(*m_socket, buffer(m_currentCmd.data(), m_currentCmd.size()),
                              std::bind(&FTPClientTask::HandleWrite, this,
                          std::placeholders::_1, std::placeholders::_2));    
      }
__RunIOContext:
    try {
      m_io->poll();        
    } catch(const system_error &ex) {
        LOG(FATAL) << "ASIO exception: " << ex.what() << ", error code " << ex.code().value();
        throw TaskRunFailed("asio exception");
    }  
  }

void FTPClientTask::Destroy() noexcept
{
    LOG(INFO) << "FTPClientTask::Run()";
    if (m_socket) {
        m_socket->close();
        m_socket = nullptr;
    }
    delete this;
}


} // namespace ftpclient
