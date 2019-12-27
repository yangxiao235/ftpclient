#ifndef _NETWORK_TASK_H
#define _NETWORK_TASK_H

#include "ftpclient/config.h"
#include "ftpclient/common_types.h"
#include "ftpclient/notify/ftp_notify_queue.h"
#include "ftpclient/cmd_task/ftp_cmd_queue.h"
#include "ftpclient/thread_model/thread_task.h"
#include "ftpclient/thread_model/thread_task_error.h"
#include <sstream>
#include <glog/logging.h>
#include <asio.hpp>
#include <string>
#include <cstdio>
#include <memory>

namespace ftpclient {

using asio::ip::tcp;
using asio::ip::address;
using asio::error_code;
using asio::system_error;
using asio::io_context;
using asio::buffer;


template <class NotifyMsgPolicy>
class FTPClientTask : public ThreadTask, public NotifyMsgPolicy
{
public:
    FTPClientTask(const std::string &ip, uint16_t port,
                       CmdQueue::CmdQueueProxy cmdQueue)
        :m_ip(ip),
         m_port(port),
         m_cmdQueue(cmdQueue)
    {

    } 
public:
    void Init() override;
    void Run()  override;
    void Destroy() noexcept override;
protected:
    void StartConnect();
    void HandleConnect(const error_code &ec, const tcp::endpoint &endpoint);
    void HandleRead(const error_code &error, size_t bytesRead);
    void HandleWrite(const error_code &error, size_t bytesWrite);
protected:
    ~FTPClientTask() {}
private:
    std::string m_ip;
    uint16_t    m_port;
    SocketPointer m_socket; 
    IOContextPointer m_io;   
    size_t      m_recvBufSize = 0;
    std::shared_ptr<std::vector<char>> m_recvBuf;
    std::string m_currentCmd;
    CmdQueue::CmdQueueProxy m_cmdQueue;
    bool        m_connected = false;
};


template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::Init() 
  {
      LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::Init()";
      m_io = std::make_shared<io_context>();
      m_socket = std::make_shared<tcp::socket>(*m_io);
      m_recvBuf = std::move(std::make_unique<std::vector<char>>(512));
      StartConnect();
  }

template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::StartConnect()
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::StartConnect()" << ", ip: " << m_ip << ", port: " << m_port;
    std::vector<tcp::endpoint> endpoints = {tcp::endpoint{address::from_string(m_ip), m_port}};
    asio::async_connect(*m_socket, endpoints, std::bind(&FTPClientTask<NotifyMsgPolicy>::HandleConnect, 
                          this, std::placeholders::_1, std::placeholders::_2));
}

template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::HandleConnect(const error_code &error, const tcp::endpoint &endpoint)
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::HandleConnect()";
    if (!error) {
      m_connected = true;
      ReportCmdNetworkConnect(m_ip, m_port);
      m_socket->async_read_some(buffer(m_recvBuf->data(), m_recvBuf->size()),
                      std::bind(&FTPClientTask<NotifyMsgPolicy>::HandleRead, this,
                      std::placeholders::_1, std::placeholders::_2));
    } else {
      ReportCmdNetworkError(error.message());
      LOG(ERROR) << "Network error:" << error.message() << ", error code: " << error.value();
      m_socket->close();
      throw TaskRunFailed("Network connect error");
    }
}

template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::HandleRead(const error_code &error, size_t bytesRead)
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::HandleRead()";
    if (!error) {
      LOG(INFO) << std::string(m_recvBuf->data(), bytesRead);
      ReportFTPCmdRecvData(std::string(m_recvBuf->data(), bytesRead));    
      m_socket->async_read_some(buffer(m_recvBuf->data(), m_recvBuf->size()),
                      std::bind(&FTPClientTask<NotifyMsgPolicy>::HandleRead, this,
                      std::placeholders::_1, std::placeholders::_2));
      
    } else {
      // 网络连接错误
      m_connected = false;
      ReportCmdNetworkError(error.message());
      LOG(ERROR) << "Network error:" << error.message() << ", error code: " << error.value();
      m_socket->close();
      throw TaskRunFailed("Network Read error");
    }
}

template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::HandleWrite(const error_code &error, size_t bytesWrite)
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::HandleWrite()";
    if (!error) {
      if (bytesWrite != m_currentCmd.size()) {
          // 记录该错误
          LOG(WARNING) << "FTPClientTask<NotifyMsgPolicy>::HandleWrite(): bytesWrite != m_currentCmd->data.size()";
      }
    } else {
      // 处理网路错误
      m_connected = false;
      ReportCmdNetworkError(error.message());
      LOG(ERROR) << "Network error:" << error.message() << ", error code: " << error.value();
      m_socket->close(); 
      throw TaskRunFailed("Network write error");
    }
}
  
template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::Run()
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::Run()";
    if (m_cmdQueue.IsEmpty() || !m_connected) {
      goto __RunIOContext ;
    }
    m_currentCmd = m_cmdQueue.Front();
    m_cmdQueue.Dequeue();
    if (!m_currentCmd.empty()) {
      asio::async_write(*m_socket, buffer(m_currentCmd.data(), m_currentCmd.size()),
                          std::bind(&FTPClientTask<NotifyMsgPolicy>::HandleWrite, this,
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

template <class NotifyMsgPolicy>
void FTPClientTask<NotifyMsgPolicy>::Destroy() noexcept
{
    LOG(INFO) << "FTPClientTask<NotifyMsgPolicy>::Run()";
    if (m_socket) {
        m_socket->close();
        m_socket = nullptr;
    }
    delete this;
}


} // end of namespace ftpclient
#endif  //  _NETWORK_TASK_H
