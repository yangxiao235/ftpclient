#ifndef _NETWORK_TASK_H
#define _NETWORK_TASK_H

#include <asio.hpp>
#include <string>
#include <cstdio>
#include <memory>



namespace network_task {

using asio::ip::tcp;
using asio::ip::address;
using asio::io_context;
using asio::error_code;
using asio::system_error;

using IOContextPointer = std::shared_ptr<io_context>;
using SocketPointer = std::shared_ptr<tcp::socket>;

class ConnectServer
{
public:
    ConnectServer(IOContextPointer io, const std::string &ip, uint16_t port)
        :m_ip(ip),
         m_port(port),
         m_io(io),
         m_socket(std::make_shared<tcp::socket>(*io))
    {

    } 
    ConnectServer(const ConnectServer &rhs) = default;
    ConnectServer &operator=(const ConnectServer &rhs)= default;

    void operator()()
    {
        try {
            StartConnect();
            m_io->run();
        } catch (const system_error &ex) {
            fprintf(stderr, "ASIO Exception: %s, error code: %d, %s\n",
                             ex.what(), ex.code().value(), ex.code().message().c_str());
        }
    }
protected:
    void StartConnect()
    {
        std::vector<tcp::endpoint> endpoints = {tcp::endpoint{address::from_string(m_ip), m_port}};
        asio::async_connect(*m_socket, endpoints, std::bind(&ConnectServer::DoConnect, 
                                this, std::placeholders::_1, std::placeholders::_2));
    }
    void DoConnect(const error_code &ec, const tcp::endpoint &endpoint)
    {
        fprintf(stderr, "Connecting to server %s:%d", m_ip.c_str(), m_port);
        m_socket->close();
    }
private:
    std::string m_ip;
    uint16_t    m_port;
    SocketPointer m_socket; 
    IOContextPointer m_io;   
};

} // end of namespace network_task

#endif  //  _NETWORK_TASK_H