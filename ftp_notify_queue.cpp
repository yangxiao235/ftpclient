#include "ftp_notify_queue.h"
#include <cassert>

namespace ftpclient {
NotifyQueue& NotifyQueue::GetInstance()
{
    static NotifyQueue *instance = nullptr;
    if (!instance) {
        static std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new NotifyQueue();
        }
    }
    return *instance;
}

void NotifyQueue::Enqueue(const Notify &notify)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(notify);
}

void NotifyQueue::Dequeue()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    m_queue.pop();
}

Notify NotifyQueue::Front()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    return m_queue.front();
}

bool NotifyQueue::IsEmpty()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.empty();
}

}// namespace ftpclient

