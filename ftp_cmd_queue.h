#ifndef _FTP_CMD_QUEUE_H
#define _FTP_CMD_QUEUE_H

#include "ftp_cmd.h"

namespace ftpclient {

class FTPCmdQueue
{
public:
    static FTPCmdQueue & GetInstance();
public:
    void Enqueue(const FTPCmdGroupPointer &);
    void Dequeue();
    FTPCmdGroupPointer Front();
    bool IsEmpty();
    ~FTPCmdQueue() = default;
private:    
    FTPCmdQueue() = default;
    FTPCmdQueue(const FTPCmdQueue&) = delete;
    FTPCmdQueue &operator=(const FTPCmdQueue&) = delete;
    std::mutex  m_accessMutex;
    std::queue<FTPCmdGroupPointer> m_queue;     
};

} // namespace ftpclient

#endif // _FTP_CMD_QUEUE_H
