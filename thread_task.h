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
    // ִ�������ʼ��
    // ʧ��: �׳�TaskInitFailed�쳣
    virtual void Init() = 0;
    // ��������
    // �������: �׳�TaskRunComplete�쳣
    // ����ʧ��: �׳�TaskRunFailed�쳣
    virtual void Run() = 0;
    // ������ɻ���ʧ�ܺ�ִ�е�������
    // �൱����������
    virtual void Destroy() noexcept = 0;
protected:
    // ������Ҫ�Ӷ��з���
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