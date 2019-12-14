#ifndef _THREAD_TASK_QUEUE_H
#define _THREAD_TASK_QUEUE_H

#include "thread_task.h"
#include <mutex>
#include <queue>
#include <cassert>

namespace ftpclient {

class TaskQueue
{
public:
    class TaskQueueProxy;
    static TaskQueueProxy Create();
protected:
    TaskQueue() = default;
public:
    ~TaskQueue() = default;
    void Enqueue(const ThreadTask::ThreadTaskProxy &task);
    void Dequeue();
    ThreadTask::ThreadTaskProxy Front();
    bool IsEmpty();
private:
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue &operator=(const TaskQueue&) = delete;
private:    
    std::mutex  m_accessMutex;
    std::queue<ThreadTask::ThreadTaskProxy> m_queue;
};

class TaskQueue::TaskQueueProxy
{
public:
    TaskQueueProxy() = default;
protected:
    friend TaskQueueProxy TaskQueue::Create();
    TaskQueueProxy(TaskQueue *queue)
        :m_queue(queue)
    {
        assert(m_queue);
    } 
public:
    TaskQueueProxy(const TaskQueueProxy & ) = default;
    TaskQueueProxy &operator=(const TaskQueueProxy &) = default;
    ~TaskQueueProxy() = default;
public:
    void Enqueue(const ThreadTask::ThreadTaskProxy &task)  
    {
        assert(m_queue);
        m_queue->Enqueue(task); 
    }
    void Dequeue() 
    { 
        assert(m_queue);
        m_queue->Dequeue(); 
    }
    ThreadTask::ThreadTaskProxy Front() 
    { 
        assert(m_queue);
        return m_queue->Front(); 
    }
    bool IsEmpty() 
    { 
        assert(m_queue);
        return m_queue->IsEmpty(); 
    }        
private:
    std::shared_ptr<TaskQueue> m_queue;
};

} // namespace ftpclient

#endif // _THREAD_TASK_QUEUE_H
