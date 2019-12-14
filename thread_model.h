#ifndef _THREAD_MODEL_H
#define _THREAD_MODEL_H

#include "thread_state.h"
#include "thread_task_queue.h"

namespace ftpclient
{

class TaskBasedThread
{
public:
    TaskBasedThread() = default;
    // 线程函数原型: void func(TaskBasedThread &, ...)
    template <class Function, class... Args>
    static TaskBasedThread Start(Function &&func, Args&&... args) 
    {
        TaskBasedThread threadInfo; 
        threadInfo.state = ThreadState::Create();
        threadInfo.queue = TaskQueue::Create();
        std::thread t{func, threadInfo, args...};
        t.detach();
        return threadInfo;
    }
    // 使用默认线程函数
    static TaskBasedThread Start(TaskBasedThread);
    static TaskBasedThread Start();
    ThreadState::ThreadStateProxy state;
    TaskQueue::TaskQueueProxy     queue;
};

class DefaultThreadFunc
{
public:    
    DefaultThreadFunc() = default;
    DefaultThreadFunc(const DefaultThreadFunc &) = default;
    DefaultThreadFunc &operator=(const DefaultThreadFunc &) = default;
    ~DefaultThreadFunc() = default;
public:    
    void operator()(TaskBasedThread info);
protected:
    void Init();
    void ChangeToIdle();
    void OnExit();
private:
    enum State {IDLE, TASK_WILL_INIT, TASK_WILL_RUN};    
    TaskBasedThread m_info;
    ThreadTask::ThreadTaskProxy m_task;
    State   m_state = IDLE;

};



} // namespace ftpclient



#endif
