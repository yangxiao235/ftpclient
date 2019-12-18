#include "ftpclient/notify/ftp_notify_queue.h"
#include "ftpclient/ftp.h"
#include <iostream>

using namespace ftpclient;

int main()
{
    {
        Notify  msg(MsgType::NETWORK_ERROR);
        Notify newmsg(msg);
    }

    {
        Reply  reply {"200", "command ok\r\n"};
        Notify  msg(MsgType::FTP_REPLY);
        msg.reply = reply;
        Notify newmsg(msg);
        std::cout << newmsg.reply.code << " " << newmsg.reply.detail << std::endl;
    }
}
