#ifndef _THREAD_TASK_H
#define _THREAD_TASK_H

#include <memory>

namespace ftpclient
{
class ThreadTask
{
public:
    class ThreadTaskProxy;
    static ThreadTaskProxy Create(ThreadTask *rawTask);
public:
    // 执行任务初始化
    // 失败: 抛出TaskInitFailed异常
    virtual void Init() = 0;
    // 运行任务
    // 任务完成: 抛出TaskRunComplete异常
    // 任务失败: 抛出TaskRunFailed异常
    virtual void Run() = 0;
    // 任务完成或者失败后执行的清理步骤
    // 相当于析构函数
    virtual void Destroy() noexcept = 0;
protected:
    // 任务需要从堆中分配
    ~ThreadTask() = default;
};

class ThreadTask::ThreadTaskProxy
{
protected:
    friend ThreadTaskProxy ThreadTask::Create(ThreadTask *);
    ThreadTaskProxy(std::shared_ptr<ThreadTask> task)
        :m_task(task)
    {}   
public:
    ThreadTaskProxy() = default;
    ThreadTaskProxy(const ThreadTaskProxy &rhs)
        :m_task(rhs.m_task)
    {}
    ThreadTaskProxy &operator=(const ThreadTaskProxy &rhs)
    {
        if (this != &rhs) {
            m_task = rhs.m_task;
        }
        return *this;
    }
    ~ThreadTaskProxy() = default;
public:
    void Init() { m_task->Init(); }
    void Run()    { m_task->Run(); }
    explicit operator bool() const { return m_task != nullptr; }
    void Reset() { m_task = nullptr; }
private:
    std::shared_ptr<ThreadTask> m_task;
};

} // namespace ftpclient


#endif // _THREAD_TASK_H