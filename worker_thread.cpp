#include "worker_thread.h"

namespace worker_thread {
void TaskRunner::Run() 
{
    auto &queue = TaskQueue::GetInstance();
    Task task;
    while (!this->m_control->stop) 
    {
        if (queue.IsEmpty()) {
            continue;
        }
        task = queue.Front();
        queue.DequeueTask();
        task();
    }
    Destroy();
}

TaskQueue& TaskQueue::GetInstance()
{
    static TaskQueue *instance = nullptr;
    if (!instance) {
        std::mutex  mx;
        std::lock_guard<std::mutex> lock(mx);
        if (!instance) {
            instance = new TaskQueue();
        }
    }
    return *instance;
}

void TaskQueue::EnqueueTask(const Task &task)
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.push(task);
}

void TaskQueue::DequeueTask()
{
    std::lock_guard<std::mutex> lock(m_accessMutex);
    m_queue.pop();
}

Task TaskQueue::Front()
{
    return m_queue.front();
}

bool TaskQueue::IsEmpty()
{
    return m_queue.empty();
}

} // namespace of worker_thread