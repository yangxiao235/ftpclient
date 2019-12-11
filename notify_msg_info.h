#ifndef _NOTIFY_MSG_INFO_H
#define _NOTIFY_MSG_INFO_H

#include "common_types.h"

namespace ftpclient {
namespace notify_msg {

using ftpclient::common::SocketPointer;
using ftpclient::common::IOContextPointer;

enum class MsgType {
   /*
    *       tcp连接事件
    * 何时通知: 连接成功通知
    * 附属数据:
    *
    */
    CONNECT = 1,
   /*
    *       ftp回复
    * 
    */
    FTP_REPLY,
   /*
    *       网络错误
    */
    NETWORK_ERROR,
   /*
    *       登陆ftp服务器事件
    * 何时通知: 登陆ftp服务器成功或者失败通知
    * 附属数据:
    */
    FTP_LOGIN,
   /*
    *       目录更新事件
    *
    */
    FTP_DIR_UPDATE,
   /*
    *       单个命令执行成功
    */
    FTP_CMD_OK,
   /*
    *       命令执行失败
    */
    FTP_CMD_FAILED
    
    
};

struct NotifyMsg
{

    MsgType type;
    void   *data;
};


struct ConnectInfo 
{
    std::string   peerIP;
    uint16_t      peerPort = 0;
    std::string   localIP;
    uint16_t      localPort;
};

struct FTPCmdNotify 
{
    bool  success;
    Reply reply;
};

} // namespace notify_msg
} // namespace ftpclient

#endif // _NOTIFY_MSG_INFO_H
