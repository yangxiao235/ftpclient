#ifndef _COMMONT_TYPES_H
#define _COMMONT_TYPES_H

#include <memory>
#include <asio.hpp>

namespace ftpclient {
using asio::io_context;
using asio::ip::tcp;
using asio::ip::address;
using IOContextPointer = std::shared_ptr<io_context>;
using SocketPointer = std::shared_ptr<tcp::socket>;

} // namespace ftpclient 



#endif // _COMMONT_TYPES_H