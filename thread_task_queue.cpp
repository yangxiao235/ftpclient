#include "thread_task_queue.h"

namespace ftpclient 
{

TaskQueue::TaskQueueProxy TaskQueue::Create()
{
    return TaskQueueProxy(new TaskQueue);
}

void TaskQueue::Enqueue(const ThreadTask::ThreadTaskProxy &task)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(task);
}

void TaskQueue::Dequeue()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    m_queue.pop();
}

ThreadTask::ThreadTaskProxy TaskQueue::Front()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    assert(!m_queue.empty());
    return m_queue.front();
}

bool TaskQueue::IsEmpty()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    return m_queue.empty();
}

}
