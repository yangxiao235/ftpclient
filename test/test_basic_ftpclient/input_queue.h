#ifndef _INPUT_QUEUE_H
#define _INPUT_QUEUE_H

#include <mutex>
#include <queue>
#include <string>

namespace ftpclient {

class InputQueue
{
public:
    static InputQueue & GetInstance();
public:
    void Enqueue(const std::string &);
    void Dequeue();
    std::string Front();
    bool IsEmpty();
    ~InputQueue() = default;
private:    
    InputQueue() = default;
    InputQueue(const InputQueue&) = delete;
    InputQueue &operator=(const InputQueue&) = delete;
    
    std::mutex  m_accessMutex;
    std::queue<std::string> m_queue;     
};

} // namespace ftpclient

#endif // _INPUT_QUEUE_H

