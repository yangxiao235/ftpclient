#include "thread_task.h"

namespace ftpclient 
{

ThreadTask::ThreadTaskProxy ThreadTask::Create(ThreadTask *rawTask)
{
    auto task = std::shared_ptr<ThreadTask>(rawTask, [] (ThreadTask *ptr) { if (ptr) ptr->Destroy(); });
    return ThreadTaskProxy(task);
}


}
