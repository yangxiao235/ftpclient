#ifndef _NETWORK_TASK_H
#define _NETWORK_TASK_H

#include "ftpclient/config.h"
#include "ftpclient/common_types.h"
#include "ftpclient/notify/ftp_notify_queue.h"
#include "ftpclient/cmd_task/ftp_cmd_queue.h"
#include "ftpclient/thread_model/thread_task.h"
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

class FTPClientTask : public ThreadTask
{
public:
    FTPClientTask(const std::string &ip, uint16_t port,
                       CmdQueue::CmdQueueProxy cmdQueue, NotifyQueue &notifyQueue)
        :m_ip(ip),
         m_port(port),
         m_cmdQueue(cmdQueue),
         m_notifyQueue(notifyQueue)
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
    size_t      m_recvDataSize = 0;
    std::shared_ptr<std::vector<char>> m_recvBuf;
    std::shared_ptr<std::vector<char>> m_recvData;
    std::string m_currentCmd;
    CmdQueue::CmdQueueProxy m_cmdQueue;
    NotifyQueue  &m_notifyQueue;
    bool        m_connected = false;
};


} // end of namespace ftpclient
#endif  //  _NETWORK_TASK_H
