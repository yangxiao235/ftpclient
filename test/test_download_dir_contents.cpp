#include "ftpclient/config.h"
#include "ftpclient/data_trans_task/data_trans_related_request.h"
#include "ftpclient/notify/ftp_notify_policy.h"
#include <asio.hpp>
#include <glog/logging.h>

using namespace ftpclient;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Invalid parameters!\n");
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }    
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";

    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    
    auto io = std::make_shared<asio::io_context>();
    auto downloadDirTask = DataTransRequestProxy::Create(new DownloadDirContentRequest<NotifyPolicyForTest>(
                                    io, 
                                    "/home",
                                    ip,
                                    port));
    downloadDirTask.Start();
    while(true) {
        io->poll_one();
        if (downloadDirTask.Complete()) {
            break;
        }
    }
    return 0;
}


