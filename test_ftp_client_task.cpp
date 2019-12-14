#define GOOGLE_GLOG_DLL_DECL 
#include "thread_task.h"
#include "thread_model.h"
#include "thread_task_error.h"
#include <glog/logging.h>
#include <cstdio>
#include <string>

using namespace ftpclient;

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";
    auto state = TaskBasedThread::Start();
    state.queue.Enqueue(ThreadTask::Create(new FTPClientTask("172.125.1.2",, 21)));
    char buf[256];
    std::string line;
    while (fgets(buf, sizeof buf, stdin)) {
        line.assign(buf);
        if (line.find("stop") != std::string::npos) {
            state.state.StopThread(true);
            fprintf(stderr, "Stopping thread now...\n");
            while (!state.state.ThreadStop());
            goto __Exit;
            
        } 
    }
__Exit:
    LOG(INFO) << "Program exit..";
    return 0;
}


