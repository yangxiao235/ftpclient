#ifndef _NOTIFY_MSG_INFO_H
#define _NOTIFY_MSG_INFO_H

#include "common_types.h"

namespace ftpclient {
namespace notify_msg {

using ftpclient::common::SocketPointer;
using ftpclient::common::IOContextPointer;

enum class MsgType {
   /*
    *       tcp�����¼�
    * ��ʱ֪ͨ: ���ӳɹ�֪ͨ
    * ��������:
    *
    */
    CONNECT = 1,
   /*
    *       ftp�ظ�
    * 
    */
    FTP_REPLY,
   /*
    *       �������
    */
    NETWORK_ERROR,
   /*
    *       ��½ftp�������¼�
    * ��ʱ֪ͨ: ��½ftp�������ɹ�����ʧ��֪ͨ
    * ��������:
    */
    FTP_LOGIN,
   /*
    *       Ŀ¼�����¼�
    *
    */
    FTP_DIR_UPDATE,
   /*
    *       ��������ִ�гɹ�
    */
    FTP_CMD_OK,
   /*
    *       ����ִ��ʧ��
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
