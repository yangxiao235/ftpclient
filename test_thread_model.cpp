#define GOOGLE_GLOG_DLL_DECL 
#include "thread_task.h"
#include "thread_model.h"
#include "thread_task_error.h"
#include <glog/logging.h>
#include <cstdio>
#include <string>

namespace ftpclient 
{
class MyThreadFunc
{
public:
    MyThreadFunc() {}
    void operator()(TaskBasedThread &info)
    {
        LOG(INFO) << "Thread MyThreadFunc start...";
        m_info = info;
        m_info.state.ThreadStart(true);
        m_info.state.ThreadStop(false);
        m_info.state.ThreadIdle(true);
        while (!m_info.state.StopThread()) {
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
                    m_task.Init();
                    m_state = TASK_WILL_RUN;
                } catch(const TaskInitFailed &ex) {
                    m_task.Destroy();
                    m_task.Reset();
                    m_state = IDLE;
                    m_info.state.ThreadIdle(true);
                    LOG(INFO) << "Task init failed: " << ex.what();
                }
                break;
            case TASK_WILL_RUN:
                try {
                    m_task.Run();
                } catch (const TaskRunComplete &ex) {
                    m_task.Destroy();
                    m_task.Reset();
                    m_state = IDLE;
                    m_info.state.ThreadIdle(true);
                    LOG(INFO) << "Task Run Complete!" << ex.what();
                } catch (const TaskRunFailed &ex) {
                    m_task.Destroy();
                    m_task.Reset();
                    m_state = IDLE;
                    m_info.state.ThreadIdle(true);
                    LOG(INFO) << "Task Run failed: " << ex.what();
                }   
                break;
            }// end of swtich
        }// end of while
       LOG(INFO) << "Thread MyThreadFunc stop...";
    }
private:
    TaskBasedThread m_info;
    ThreadTask::ThreadTaskProxy m_task;
    enum State {IDLE, TASK_WILL_INIT, TASK_WILL_RUN};    
    State   m_state = IDLE;
};

class MyTask : public ThreadTask {
public:
    void Init() override
    {
        fprintf(stdout, "MyTask::Init()\n");
    }
    void Run() override
    {
        fprintf(stdout, "MyTask::Run()\n");
        throw TaskRunComplete("");
    }
    void Destroy() noexcept override
    {

    }
    
};

}

using namespace ftpclient;

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";
    auto state = TaskBasedThread::Start(MyThreadFunc{});
    state.queue.Enqueue(ThreadTask::Create(new MyTask));
    char buf[256];
    std::string line;
    while (fgets(buf, sizeof buf, stdin)) {
        line.assign(buf);
        if (line.find("stop") != std::string::npos) {
            state.state.StopThread(true);
            fprintf(stderr, "Stopping thread now...\n");
        } 
    }
}