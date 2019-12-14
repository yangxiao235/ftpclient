#include "thread_task.h"
#include <iostream>

class MyTask : public ftpclient::ThreadTask
{
public:
    MyTask()  {}
    void Init()   override {}
    // ��������
    // �������: �׳�TaskRunComplete�쳣
    // ����ʧ��: �׳�TaskRunFailed�쳣
     void Run() override 
    { 
        std::cout << "MyTask::Run()" << std::endl;
    }
    // ������ɻ���ʧ�ܺ�ִ�е�������
    // �൱����������
     void Destroy() noexcept override {}
private:
    ~MyTask();
};


int main()
{
    auto task = ftpclient::ThreadTask::Create(new MyTask);
    
}
