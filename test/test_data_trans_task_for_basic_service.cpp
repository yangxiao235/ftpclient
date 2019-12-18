#include "ftpclient/config.h"
#include "ftpclient/data_trans_task/data_trans_task.h"
#include <asio.hpp>
#include <glog/logging.h>

using namespace ftpclient;

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";
    
    auto io = std::make_shared<asio::io_context>();
    auto queue = DataTransRequestQueueProxy::Create();
    auto task = ThreadTask::Create(new DataTransTask(io, queue));
    task.Init();
    while (true)
        task.Run();
}
