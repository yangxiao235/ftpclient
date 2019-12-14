#define GOOGLE_GLOG_DLL_DECL
//#define GLOG_NO_ABBREVIATED_SEVERITIES

#include "thread_model.h"
#include "thread_task_error.h"
#include <glog/logging.h>
#include <thread>

namespace ftpclient
{
TaskBasedThread TaskBasedThread::Start(TaskBasedThread info)
{
    std::thread t{DefaultThreadFunc{}, info};
    t.detach();
    return info;
}

TaskBasedThread TaskBasedThread::Start()
{
    TaskBasedThread threadInfo; 
    threadInfo.state = ThreadState::Create();
    threadInfo.queue = TaskQueue::Create();
    std::thread t{DefaultThreadFunc{}, threadInfo};
    t.detach();
    return threadInfo;
}

void DefaultThreadFunc::operator()(TaskBasedThread info)
{
    LOG(INFO) << "Thread " << std::this_thread::get_id() <<  " start...";
    m_info = info;
    Init();
    for(;;) {
        if (m_info.state.StopThread()) {
            // 命令线程退出
            if (m_task) {
                m_task.Reset();
            }
            m_info.state.StopThread(false);
            break;
        }
        if (m_info.state.CancelTask() && m_task) {
            // 取消当前任务的执行
            m_task.Reset();
            m_info.state.CancelTask(false);
            ChangeToIdle();
        }
        if (m_info.state.RestartTask() && m_task) {
            // 重新开始任务
            // 这要求m_task能够重入: 
            //  <- ---------------------------- <-
            //  -> Init() -> Run() -> Reset() ->
            m_task.Reset();
            Init();         
            m_info.state.RestartTask(false);
        }
        switch (m_state) {
        case IDLE:
            if (!m_info.queue.IsEmpty()) {
                m_task = m_info.queue.Front();
                m_info.queue.Dequeue();
                m_info.state.ThreadIdle(false);
                m_state = TASK_WILL_INIT;
            }            
            break;
        case TASK_WILL_INIT:
            try {
                LOG(INFO) << "Task::Init()";
                m_task.Init();
                m_state = TASK_WILL_RUN;
            } catch(const TaskInitFailed &ex) {
                m_task.Reset();
                ChangeToIdle();
                LOG(WARNING) << "Task::Init() failed: " << ex.what();
            }
            break;
        case TASK_WILL_RUN:
            try {
                m_task.Run();
            } catch (const TaskRunComplete &) {
                m_task.Reset();
                ChangeToIdle();
                LOG(INFO) << "Task::Run() Complete!";
            } catch (const TaskRunFailed &ex) {
                m_task.Reset();
                ChangeToIdle();
                LOG(WARNING) << "Task::Run() failed: " << ex.what();
            }   
            break;
        }// end of swtich
    }// end of for
    OnExit();
    LOG(INFO) << "Thread " << std::this_thread::get_id() << "  stop...";    
}

void DefaultThreadFunc::Init()
{
    m_info.state.ThreadStart(true);
    m_info.state.ThreadStop(false);
    m_info.state.ThreadIdle(true);
    m_info.state.CancelTask(false);
    m_info.state.StopThread(false);
    m_info.state.RestartTask(false);
    m_state = IDLE;
}

void DefaultThreadFunc::ChangeToIdle()
{
    m_info.state.ThreadIdle(true);
    m_state = IDLE;
}

void DefaultThreadFunc::OnExit()
{
    m_info.state.ThreadStart(false);
    m_info.state.ThreadStop(true);
    m_info.state.ThreadIdle(false);
}

}
