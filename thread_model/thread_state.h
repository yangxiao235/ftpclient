#ifndef _THREAD_STATE_H
#define _THREAD_STATE_H

#include <atomic>
#include <memory>

namespace ftpclient {
class ThreadState
{
public:
    class ThreadStateProxy;
    static ThreadStateProxy Create();
protected:
    ThreadState()
    {
        std::atomic_init(&m_cancelTask , false);
        std::atomic_init(&m_restartTask, false);
        std::atomic_init(&m_stopThread , false);
        std::atomic_init(&m_threadStart, false);
        std::atomic_init(&m_threadStop , true);
        std::atomic_init(&m_threadIdle , true);
    }
    ThreadState(const ThreadState &) = delete;
    ThreadState &operator=(const ThreadState &) = delete;
public:
    ~ThreadState() = default;
// 主线程使用
    // 写
    void CancelTask(bool cancel) { m_cancelTask.store(cancel); }
    void RestartTask(bool restart) { m_restartTask.store(restart); }
    void StopThread(bool stop) {      m_stopThread.store(stop); }
    // 只读
    bool ThreadStart() { return m_threadStart.load(); }
    bool ThreadStop() { return m_threadStop.load(); }
    bool ThreadIdle() { return m_threadIdle.load(); }
// 线程自身使用
    // 写
    void ThreadStart(bool start) { m_threadStart.store(start); }
    void ThreadStop(bool stop) {     m_threadStop.store(stop); }
    void ThreadIdle(bool idle) { m_threadIdle.store(idle); }    
    // 只读
    bool CancelTask() { return m_cancelTask.load(); }
    bool RestartTask() { return      m_restartTask.load(); }
    bool StopThread()   { return m_stopThread.load(); }
private:
    std::atomic_bool m_cancelTask ;
    std::atomic_bool m_restartTask;
    std::atomic_bool m_stopThread ;
    std::atomic_bool m_threadStart;
    std::atomic_bool m_threadStop ;
    std::atomic_bool m_threadIdle ;
};

class ThreadState::ThreadStateProxy
{
public:
    ThreadStateProxy() = default;
protected:
    friend ThreadStateProxy ThreadState::Create();
    ThreadStateProxy(ThreadState *state)
        :m_state(state)
    {}    
public:
    ThreadStateProxy(const ThreadStateProxy& ) = default;
    ThreadStateProxy &operator=(const ThreadStateProxy &) = default;
public:
// 主线程使用
    // 写
    void CancelTask(bool cancel) { m_state->CancelTask(cancel); }
    void RestartTask(bool restart) { m_state->RestartTask(restart); }
    void StopThread(bool stop) { m_state->StopThread(stop); }
    // 只读
    bool ThreadStart() { return m_state->ThreadStart(); }
    bool ThreadStop()       { return m_state->ThreadStop(); }
    bool ThreadIdle()       { return m_state->ThreadIdle(); }
// 线程自身使用
    // 写
    void ThreadStart(bool start) { m_state->ThreadStart(start); }
    void ThreadStop(bool stop) { m_state->ThreadStop(stop); }
    void ThreadIdle(bool idle) { m_state->ThreadIdle(idle); }    
    // 只读
    bool CancelTask() { return m_state->CancelTask(); }
    bool RestartTask() { return m_state->RestartTask(); }
    bool StopThread()  { return m_state->StopThread(); } 
private:
    std::shared_ptr<ThreadState> m_state;
};


} // namespace ftpclient
#endif // _THREAD_STATE_H
