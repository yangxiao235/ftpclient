#include "ftpclient/config.h"
#include "ftpclient/ftp_error.h"
#include "ftpclient/cmd_task/network_task.h"
#include "ftpclient/thread_model/thread_task_queue.h"
#include "ftpclient/notify/ftp_notify_queue.h"
#include "ftpclient/thread_model/thread_model.h"
#include "monitor_stdio_task.h"
#include <cstdio>
#include <string>
#include <glog/logging.h>

using namespace ftpclient;
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";

    // 创建cmd队列, notify队列
    auto cmdQueue = CmdQueue::Create();
    auto &notifyQueue = NotifyQueue::GetInstance();
    auto &inputQueue = InputQueue::GetInstance();
    // 创建网络线程
    auto netThread = TaskBasedThread::Start();
    // 创建io监视线程
    auto ioThread = TaskBasedThread::Start();
    // 创建网络线程任务
    netThread.queue.Enqueue(ThreadTask::Create(new FTPClientTask("172.125.1.2", 21, cmdQueue, notifyQueue)));
    // 创建io监视任务
    ioThread.queue.Enqueue(ThreadTask::Create(new MonitorStdioTask(cmdQueue)));
    // 读取输入命令    
    Notify notify(MsgType::DEFAULT);
    std::string line;    
    char buf[256];
    while (fgets(buf, sizeof buf, stdin)) {
        line.assign(buf);
        inputQueue.Enqueue(line);
    }
    // 命令线程退出
    netThread.state.StopThread(true);
    ioThread.state.StopThread(true);
    // 等待线程退出
    while (netThread.state.ThreadStop() && ioThread.state.ThreadStop());
    fprintf(stderr, "Program exit\n");
}





