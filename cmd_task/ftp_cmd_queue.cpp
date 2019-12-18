#include "ftp_cmd_queue.h"
namespace ftpclient {

CmdQueue::CmdQueueProxy CmdQueue::Create()
{
    return CmdQueueProxy(new CmdQueue);
}

void CmdQueue::Enqueue(const std::string &cmd)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(cmd);
}

void CmdQueue::Dequeue()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    m_queue.pop();
}

std::string CmdQueue::Front()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    return m_queue.front();
}

bool CmdQueue::IsEmpty()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.empty();
}

}// namespace ftpclient
