#ifndef _NETWORK_TASK_H
#define _NETWORK_TASK_H

#include "common_types.h"
#include "notify_msg.h"
#include <asio.hpp>
#include <string>
#include <cstdio>
#include <memory>

namespace ftpclient {
namespace network_task {

using asio::ip::tcp;
using asio::ip::address;
using asio::error_code;
using asio::system_error;
using notify_msg::MsgType;
using notify_msg::NotifyMsg;
using notify_msg::ConnectInfo;
using notify_msg::NotifyQueue;
using ftpclient::common::IOContextPointer;
using ftpclient::common::SocketPointer;

class FTPClient
{
public:
    FTPClient(const std::string &ip, uint16_t port)
        :m_ip(ip),
         m_port(port)
    {

    } 
    FTPClient(const FTPClient &rhs) = default;
    FTPClient &operator=(const FTPClient &rhs)= default;

    void operator()()
    {
        try { 
            if (!m_init) {
                Init();
            }
            Run();
        } catch (const system_error &ex) {
            fprintf(stderr, "ASIO Exception: %s, error code: %d, %s\n",
                             ex.what(), ex.code().value(), ex.code().message().c_str());
        }
    }
protected:
    void Init() ;
    void StartConnect();
    void DoConnect(const error_code &ec, const tcp::endpoint &endpoint);
    void HandleRead(const error_code &error, size_t bytesRead);
    void HandleWrite(const error_code &error, size_t bytesWrite);
    void Run();
private:
    bool        m_init = false;
    std::string m_ip;
    uint16_t    m_port;
    SocketPointer m_socket; 
    IOContextPointer m_io;   
    size_t      m_recvBufSize = 0;
    size_t      m_recvDataSize = 0;
    std::unique_ptr<std::vector<char>> m_recvBuf;
    std::unique_ptr<std::vector<char>> m_recvData;
    FTPCmdGroupPointer m_currentCmdGroup;
    FTPCmdGroupPointer m_defaultCmdGroup;
    FTPCmdGroup::const_iterator m_currentCmd;
};


} // end of namespace network_task
} // end of namespace ftpclient
#endif  //  _NETWORK_TASK_H
