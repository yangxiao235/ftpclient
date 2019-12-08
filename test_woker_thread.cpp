#include "worker_thread.h"
#include <thread>
#include <cstdio>
#include <string>

struct MyTask 
{
    MyTask (const std::string &word_)
        :word(word_)
    {}
    void operator()() {
        fprintf(stderr, "%s\n", word.c_str());
    }
    
    std::string word;
};
using namespace worker_thread;

int main()
{
    auto control = TCBPointer{new ThreadControlBlock,
                              [] (ThreadControlBlock *ptr) { ptr->Destroy();} };
    auto taskRunner = TaskRunnerPointer{new TaskRunner{control},
                              [] (TaskRunner *ptr) { ptr->Destroy();} };
    std::thread wokerThread{std::bind(&TaskRunner::Run, std::move(taskRunner))};
    wokerThread.detach();
    taskRunner = nullptr;

    auto &queue = TaskQueue::GetInstance();
    queue.EnqueueTask(MyTask{"word1"});
    char line[80];
    while (fgets(line, sizeof(line), stdin)) {
        queue.EnqueueTask(MyTask{line});
    }
}