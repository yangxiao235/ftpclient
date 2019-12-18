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

    // ����cmd����, notify����
    auto cmdQueue = CmdQueue::Create();
    auto &notifyQueue = NotifyQueue::GetInstance();
    auto &inputQueue = InputQueue::GetInstance();
    // ���������߳�
    auto netThread = TaskBasedThread::Start();
    // ����io�����߳�
    auto ioThread = TaskBasedThread::Start();
    // ���������߳�����
    netThread.queue.Enqueue(ThreadTask::Create(new FTPClientTask("172.125.1.2", 21, cmdQueue, notifyQueue)));
    // ����io��������
    ioThread.queue.Enqueue(ThreadTask::Create(new MonitorStdioTask(cmdQueue)));
    // ��ȡ��������    
    Notify notify(MsgType::DEFAULT);
    std::string line;    
    char buf[256];
    while (fgets(buf, sizeof buf, stdin)) {
        line.assign(buf);
        inputQueue.Enqueue(line);
    }
    // �����߳��˳�
    netThread.state.StopThread(true);
    ioThread.state.StopThread(true);
    // �ȴ��߳��˳�
    while (netThread.state.ThreadStop() && ioThread.state.ThreadStop());
    fprintf(stderr, "Program exit\n");
}





