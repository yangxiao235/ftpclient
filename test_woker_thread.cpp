#include "network_task.h"
#include "worker_thread.h"
#include "notify_msg.h"
#include <thread>
#include <cstdio>
#include <string>
#include <memory>
#include <chrono>

using namespace ftpclient::worker_thread;
using namespace ftpclient::network_task;
using namespace ftpclient::notify_msg;
using asio::io_context;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }
    auto control = TCBPointer{new ThreadControlBlock,
                              [] (ThreadControlBlock *ptr) { ptr->Destroy();} };
    auto taskRunner = TaskRunnerPointer{new TaskRunner{control},
                              [] (TaskRunner *ptr) { ptr->Destroy();} };
    std::thread wokerThread{std::bind(&TaskRunner::Run, std::move(taskRunner))};
    wokerThread.detach();
    taskRunner = nullptr;

    auto &queue = TaskQueue::GetInstance();
    auto io = std::make_shared<io_context>();
    std::string serverIP{ argv[1] };
    uint16_t serverPort = static_cast<uint16_t>(std::stoi(argv[2]));
    queue.EnqueueTask(ConnectServer{io, serverIP, serverPort});

    auto &notifyQueue = NotifyQueue::GetInstance();
    NotifyMsg  msg;
    for (;;) {
        if (!notifyQueue.IsEmpty()) {
            msg = notifyQueue.Front();
            notifyQueue.DequeueMsg();
            if (msg.type == MsgType::CONNECT) {
                auto info = reinterpret_cast<ConnectInfo *>(msg.data);
                if (info->success) {
                    fprintf(stderr, "Connection to %s:%d\n", serverIP.c_str(), serverPort);
                } else {
                    fprintf(stderr, "Connection failed to establish. %s\n", info->errmsg.c_str());
                }
                delete info;
                info = nullptr;
                msg.data = nullptr;
            }

        }

    }

}