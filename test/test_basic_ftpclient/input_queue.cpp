#include "input_queue.h"
#include <cassert>

namespace ftpclient {
InputQueue& InputQueue::GetInstance()
{
    static InputQueue *instance = nullptr;
    if (!instance) {
        static std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new InputQueue();
        }
    }
    return *instance;
}

void InputQueue::Enqueue(const std::string &line)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(line);
}

void InputQueue::Dequeue()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    m_queue.pop();
}

std::string InputQueue::Front()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    return m_queue.front();
}

bool InputQueue::IsEmpty()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.empty();
}

}// namespace ftpclient


