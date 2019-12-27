#include "ftpclient/config.h"
#include "ftpclient/data_trans_task/data_trans_related_request.h"
#include "ftpclient/data_trans_task/file_save_policy.h"
#include "ftpclient/notify/ftp_notify_policy.h"
#include <asio.hpp>
#include <glog/logging.h>

using namespace ftpclient;

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Invalid parameters!\n");
        fprintf(stderr, "Usage: %s <filename> <ip> <port>\n", argv[0]);
        return -1;
    }    
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = ".";

    std::string filename = argv[1];
    std::string ip = argv[2];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));
    
    auto io = std::make_shared<asio::io_context>();
    auto downloadFileTask = DataTransRequestProxy::Create(new DownloadFileRequest<SimpleFileSave,NotifyPolicyForTest>(
                                    io, 
                                    filename,
                                    "null",
                                    ip,
                                    port));
    downloadFileTask.Start();
    while(true) {
        io->poll_one();
        if (downloadFileTask.Complete()) {
            break;
        }
    }
    return 0;
}


