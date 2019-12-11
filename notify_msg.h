#ifndef _NOTIFY_MSG_H
#define _NOTIFY_MSG_H

#include "notify_msg_info.h"
#include <queue>
#include <mutex>

namespace ftpclient {
namespace notify_msg {

class NotifyQueue
{
public:
    static NotifyQueue & GetInstance();
public:
    void EnqueueMsg(const NotifyMsg &);
    void DequeueMsg();
    NotifyMsg Front();
    bool IsEmpty();
    ~NotifyQueue() = default;
private:    
    NotifyQueue() = default;
    NotifyQueue(const NotifyQueue&) = delete;
    NotifyQueue &operator=(const NotifyQueue&) = delete;
    std::mutex  m_accessMutex;
    std::queue<NotifyMsg> m_queue;
};

} // namespace notify_msg
} // namespace ftpclient
#endif // _NOTIFY_MSG_H
