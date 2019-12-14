#ifndef _THREAD_TASK_ERROR_H
#define _THREAD_TASK_ERROR_H

#include <stdexcept>

namespace ftpclient 
{
class TaskInitFailed : public std::runtime_error
{
public:
    explicit TaskInitFailed(const std::string &errmsg)
        :std::runtime_error(errmsg)
    {}
    explicit TaskInitFailed(const       char *errmsg)
        :std::runtime_error(errmsg)
    {}    
    ~TaskInitFailed() = default;

    TaskInitFailed(const TaskInitFailed &) = default;
    TaskInitFailed &operator=(const TaskInitFailed &) = default;
};



class TaskRunFailed : public std::runtime_error
{
public:
    explicit TaskRunFailed(const std::string &errmsg)
        :std::runtime_error(errmsg)
    {}
    explicit TaskRunFailed(const       char *errmsg)
        :std::runtime_error(errmsg)
    {}    
    ~TaskRunFailed() = default;
    TaskRunFailed(const TaskRunFailed &) = default;
    TaskRunFailed &operator=(const TaskRunFailed &) = default;
};




class TaskRunComplete : public std::runtime_error
{
public:
    explicit TaskRunComplete(const std::string &errmsg)
        :std::runtime_error(errmsg)
    {}
    explicit TaskRunComplete(const       char *errmsg)
        :std::runtime_error(errmsg)
    {}    
    ~TaskRunComplete() = default;
    TaskRunComplete(const TaskRunComplete &) = default;
    TaskRunComplete &operator=(const TaskRunComplete &) = default;
};

} // namespace ftpclient

#endif // _THREAD_TASK_ERROR_H
