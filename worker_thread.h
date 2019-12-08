#ifndef _WOKER_THREAD_H
#define _WOKER_THREAD_H

#include <atomic>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>

namespace worker_thread {
class ThreadControlBlock
{
public:    
    std::atomic_bool  stop = false;
    void Destroy() { delete this; }
private:
    ~ThreadControlBlock() = default;
};
using TCBPointer = std::shared_ptr<ThreadControlBlock>;

using Task = std::function<void()>;
class TaskQueue
{
public:
    static TaskQueue& GetInstance();
protected:
    TaskQueue() = default;
public:
    ~TaskQueue() = default;
    void EnqueueTask(const Task &);
    void DequeueTask();
    Task Front();
    bool IsEmpty();
private:
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue &operator=(const TaskQueue&) = delete;
    std::mutex  m_accessMutex;
    std::queue<Task> m_queue;
};

class TaskRunner
{
public:
    TaskRunner(TCBPointer control)
        :m_control(control)
    {}
    void Run();
    void Destroy() { delete this; }
private:
    ~TaskRunner() = default;
private:
    TCBPointer m_control;
};

using TaskRunnerPointer = std::unique_ptr<TaskRunner, void (*)(TaskRunner *)>;

} // namespace worker_thread

#endif // _WOKER_THREAD_H