#include "ftpclient/config.h"
#include "ftpclient/data_trans_task/data_trans_task.h"
#include "ftpclient/data_trans_task/data_trans_request_queue.h"
#include "ftpclient/data_trans_task/data_trans_related_request.h"
#include <glog/logging.h>
#include <asio.hpp>

using namespace ftpclient;
using Request = DownloadFileRequest<std::string, SimpleFileSavePolicy>;
int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";
    
    auto io = std::make_shared<asio::io_context>();
    auto requestQqueue = DataTransRequestQueueProxy::Create();
    auto task = ThreadTask::Create(new DataTransTask(io, requestQqueue));
    auto downFileReq = DataTransRequestProxy::Create(new Request(io, ".\\test\\down.txt", "127.0.0.1", 9000));
    requestQqueue.Enqueue(downFileReq);
    
    task.Init();
    while (true)
        task.Run();
}
