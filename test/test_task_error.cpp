#include "ftpclient/thread_model/thread_task_error.h"
#include <iostream>

using namespace ftpclient;

void fun()
{
    throw TaskRunFailed("Exception taskrunfailed.");
}

int main()
{
    try {
        fun();
    } catch( const TaskRunFailed &ex)
    {
        std::cout << ex.what() << std::endl;
    }

}