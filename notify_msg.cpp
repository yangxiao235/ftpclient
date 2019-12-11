#include "notify_msg.h"

namespace ftpclient {
namespace notify_msg {

NotifyQueue& NotifyQueue::GetInstance()
{
    static NotifyQueue *instance = nullptr;
    if (!instance) {
        std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new NotifyQueue();
        }
    }
    return *instance;
}

void NotifyQueue::EnqueueMsg(const NotifyMsg &msg)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(msg);
}

void NotifyQueue::DequeueMsg()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.pop();
}

NotifyMsg NotifyQueue::Front()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.front();
}

bool NotifyQueue::IsEmpty()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.empty();
}

} // namespace notify_msg
} // namespace ftpclient