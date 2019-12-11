#include "network_task.h"
#include <glog/logging.h>
#include <sstream>

namespace ftpclient {
namespace network_task {

void FTPClient::Init() 
  {
      m_init = true;
      m_io = std::make_shared<io_context>();
      m_socket = std::make_shared<tcp::socket>(*m_io);
      m_recvBuf = std::move(std::make_unique<std::vector<char>>(512));
      m_recvData = std::move(std::make_unique<std::vector<char>>(1024));
      StartConnect();
  }
  void FTPClient::StartConnect()
  {
      std::vector<tcp::endpoint> endpoints = {tcp::endpoint{address::from_string(m_ip), m_port}};
      asio::async_connect(*m_socket, endpoints, std::bind(&FTPClient::DoConnect, 
                              this, std::placeholders::_1, std::placeholders::_2));
  }
  
  void FTPClient::DoConnect(const error_code &ec, const tcp::endpoint &endpoint)
  {
      NotifyMsg msg;
      auto info = new ConnectInfo();
      msg.type = MsgType::CONNECT;
      msg.data = reinterpret_cast<void *>(info);
      info->peerIP = m_ip;
      info->peerPort = m_port;
      if (!ec) {
          info->localIP = m_socket->local_endpoint().address().to_string();
          info->localPort = m_socket->local_endpoint().port();
          asio::async_read(*m_socket, buffer(m_recvBuf->data(), m_recvBuf->size()),
                          std::bind(HandleRead, this));
      } else {
          NotifyMsg msg;
          msg.type = MsgType::NETWORK_ERROR;
          msg.data = nullptr;
          NotifyQueue::GetInstance().EnqueueMsg(msg);
          LOG(ERROR) << "Network error:" << error.message() << "error code: " << error.value();
          m_socket->close();
      }
      NotifyQueue::GetInstance().EnqueueMsg(msg);
  }
  
  void FTPClient::HandleRead(const error_code &error, size_t bytesRead)
  {
      if (!error) {
          // 将本次读到的数据追加到m_recvData
          m_recvBufSize = bytesRead;
          std::memcpy(m_recvData->data() + m_recvDataSize, m_recvBuf.data(), m_recvBufSize);
          m_recvDataSize += m_recvBufSize;
          // 解析ftp回复
          FTPReply reply;
          const char *end;
          if (ParseReply(m_recvData->data(), m_recvDataSize, end, reply)) {
              // 解析完成, 将剩余的数据复制到m_recvData开头
              std::memcpy(m_recvData->data(), end, m_recvData.data() + m_recvDataSize - end);
              m_recvDataSize -= (end - m_recvData.data());
              // 通知FTP_REPLY事件
              NotifyMsg msg;
              msg.type = MsgType::FTP_REPLY;
              auto data = new FTPReply;
              *data = reply;
              msg.data = reinterpret_cast<void *>(data);
              NotifyQueue::GetInstance().EnqueueMsg(msg);
              // 继续下一个命令
              auto status = m_currentCmdGroup->OnRecieve(reply);
              switch (status) {
              case FTPCmdGroup::NOTFINISH:
                  break;
              case FTPCmdGroup::NEXT:
                  ++m_currentCmd;
                  asio::async_write(m_socket, buffer(m_currentCmd->data.data(), m_currentCmd->data.size()),
                                        std::bind(HandleWrite, this));    
                  break;
              case FTPCmdGroup::FINISH:
                   m_currentCmdGroup = m_defaultCmdGroup;
                   m_currentCmd = m_defaultCmdGroup->cbegin();
              } // end of switch
          } else if (m_recvDataSize >= 512) {
              // ftp回复格式错误
              LOG(FATAL) << "Invalid FTP reply.";
          }
      } else {
          // 网络连接错误
          NotifyMsg msg;
          msg.type = MsgType::NETWORK_ERROR;
          msg.data = nullptr;
          NotifyQueue::GetInstance().EnqueueMsg(msg);
          LOG(ERROR) << "Network error:" << error.message() << "error code: " << error.value();
          m_socket->close();
      }
  }

  void FTPClient::HandleWrite(const error_code &error, size_t bytesWrite)
  {
      if (!error) {
          if (bytesWrite != m_currentCmd->data.size()) {
              // 记录该错误
              LOG(WARNING) << "FTPClient::HandleWrite(): bytesWrite != m_currentCmd->data.size()";
          }
      } else {
          // 处理网路错误
          NotifyMsg msg;
          msg.type = MsgType::NETWORK_ERROR;
          msg.data = nullptr;
          NotifyQueue::GetInstance().EnqueueMsg(msg);
          LOG(ERROR) << "Network error:" << error.message() << "error code: " << error.value();
          m_socket->close();         
      }
  }
  
  void FTPClient::Run()
  {
      auto &cmdQueue = ftpclient::FTPCmdQueue();
      if ((m_currentCmd == m_defaultCmdGroup) && cmdQueue.IsEmpty()) {
          return ;
      }
      if (m_currentCmd != m_defaultCmdGroup) {
          return;
      }
      m_currentCmdGroup = cmdQueue.Front();
      cmdQueue.Dequeue();
      m_currentCmd = m_currentCmdGroup.cbegin();
      if (m_currentCmd->data.empty()) {
          asio::async_write(m_socket, buffer(m_currentCmd->data.data(), m_currentCmd->data.size()),
                              std::bind(HandleWrite, this));    
      }
  }


} // namespace network_task
} // namespace ftpclient
