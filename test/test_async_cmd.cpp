#include "ftpclient/config.h"
#include "ftpclient/async_cmd/async_cmd.h"
#include "ftpclient/ftp_error.h"
#include "ftpclient/cmd_task/network_task.h"
#include "ftpclient/cmd_task/ftp_cmd_queue.h"
#include "ftpclient/notify/ftp_notify_queue.h"
#include "ftpclient/thread_model/thread_task_queue.h"
#include "ftpclient/thread_model/thread_model.h"
#include <cstdio>
#include <sstream>
#include <glog/logging.h>

namespace ftpclient 
{

void MyHandler(const std::error_code &ec)
{
    if (!ec) {
        fprintf(stderr, "%s\n", ec.message().c_str());
    } else {
        fprintf(stderr, "FTP cmd success\n");
    }
    
}

class MyAsyncCmd : public AsyncCmd
{
public:
    MyAsyncCmd(const std::vector<std::string> &cmdAndArgs, CmdQueue::CmdQueueProxy cmdQueue)
        :m_cmdLine(cmdAndArgs),
        m_cmdQueue(cmdQueue)
    {}    
    ~MyAsyncCmd() = default;
    void Start() override
    {
        std::ostringstream sout;
        auto it = m_cmdLine.cbegin();
        sout << *it++;
        for (; it != m_cmdLine.cend(); ++it) {
            sout << " " << *it;
        }
        sout << "\r\n";
        m_cmdQueue.Enqueue(sout.str());
    }
    bool OnRecieved(const Notify &notify, std::error_code &ec) override
    {
        if (notify.type == MsgType::NETWORK_ERROR) {
            ec.assign(static_cast<int>(FTPError::NETWORK_ERROR), FTPErrorCategory::GetInstance());
            return true;
        }
        if (notify.type == MsgType::FTP_REPLY) {
            fprintf(stderr, notify.reply.detail.c_str());
            if (notify.reply.code[0] == '2') {
                ec.clear();
                return true;
            } else {
                ec.assign(static_cast<int>(FTPError::FTP_REQUEST_REFUSED), FTPErrorCategory::GetInstance());
                return true;
            }
        }
    }
private:
    std::vector<std::string> m_cmdLine;
    CmdQueue::CmdQueueProxy  m_cmdQueue;
};
}

using namespace ftpclient;
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";

    // 创建cmd队列, notify队列
    auto cmdQueue = CmdQueue::Create();
    auto &notifyQueue = NotifyQueue::GetInstance();
    auto cmd = AsyncCmd::Create(new MyAsyncCmd{{"NOOP"}, cmdQueue});
    // 创建线程
    auto threadInfo = TaskBasedThread::Start();
    // 创建任务并添加到线程任务队列中
    threadInfo.queue.Enqueue(ThreadTask::Create(new FTPClientTask("172.125.1.2", 21, cmdQueue, notifyQueue)));
    // 创建异步命令
    auto ch = CmdAndHandlerPair{cmd, MyHandler};
    // 执行异步命令
    ch.first.Start();
    Notify notify(MsgType::DEFAULT);
    std::error_code ec;
    for (;;) {
        if (!notifyQueue.IsEmpty()) {
            notify = notifyQueue.Front();
            notifyQueue.Dequeue();
            fprintf(stderr, "recieved notify\n");
            if (notify.type == MsgType::NETWORK_ERROR ||
                notify.type == MsgType::FTP_REPLY) {
                if (ch.first.OnRecieved(notify, ec)) {
                    fprintf(stderr, "Cmd complete.\n");
                }
            }
        }
    }
}




