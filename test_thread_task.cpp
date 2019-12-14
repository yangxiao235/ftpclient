#include "thread_task.h"
#include <iostream>

class MyTask : public ftpclient::ThreadTask
{
public:
    MyTask()  {}
    void Init()   override {}
    // 运行任务
    // 任务完成: 抛出TaskRunComplete异常
    // 任务失败: 抛出TaskRunFailed异常
     void Run() override 
    { 
        std::cout << "MyTask::Run()" << std::endl;
    }
    // 任务完成或者失败后执行的清理步骤
    // 相当于析构函数
     void Destroy() noexcept override {}
private:
    ~MyTask();
};


int main()
{
    auto task = ftpclient::ThreadTask::Create(new MyTask);
    
}
