#include "network_task.h"
#include "worker_thread.h"
#include <thread>
#include <cstdio>
#include <string>
#include <memory>
#include <chrono>

using namespace worker_thread;
using namespace network_task;
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
    queue.EnqueueTask(ConnectServer{io, argv[1], static_cast<uint16_t>(std::stoi(argv[2]))});
    std::this_thread::sleep_for(std::chrono::seconds{ 3 });

}