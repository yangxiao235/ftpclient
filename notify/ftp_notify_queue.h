#ifndef _FTP_NOTIFY_QUEUE_H
#define _FTP_NOTIFY_QUEUE_H

#include "ftp_notify_msg.h"
#include "ftpclient/ftp.h"
#include <queue>
#include <mutex>

namespace ftpclient
{
class NotifyQueue
{
public:
    static NotifyQueue & GetInstance();
public:
    void Enqueue(const Notify &);
    void Dequeue();
    Notify Front();
    bool IsEmpty();
    ~NotifyQueue() = default;
private:    
    NotifyQueue() = default;
    NotifyQueue(const NotifyQueue&) = delete;
    NotifyQueue &operator=(const NotifyQueue&) = delete;
    
    std::mutex  m_accessMutex;
    std::queue<Notify> m_queue; 
};


}// namespace ftpclient


#endif // _FTP_NOTIFY_QUEUE_H
