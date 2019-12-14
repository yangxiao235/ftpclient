#ifndef _FTP_CMD_QUEUE_H
#define _FTP_CMD_QUEUE_H

#include <string>
#include <queue>
#include <mutex>
#include <cassert>

namespace ftpclient {

class CmdQueue
{
public:
    class CmdQueueProxy;
    static CmdQueueProxy Create();
public:
    void Enqueue(const std::string &);
    void Dequeue();
    std::string Front();
    bool IsEmpty();
    ~CmdQueue() = default;
private:    
    CmdQueue() = default;
    CmdQueue(const CmdQueue&) = delete;
    CmdQueue &operator=(const CmdQueue&) = delete;
    std::mutex  m_accessMutex;
    std::queue<std::string> m_queue;     
};

class CmdQueue::CmdQueueProxy
{
public:
    CmdQueueProxy() = default;
protected:
    friend CmdQueueProxy CmdQueue::Create();
    CmdQueueProxy(CmdQueue *queue)
        :m_queue(queue)
    {
        assert(m_queue);
    }    
public:
    void Enqueue(const std::string &cmd) 
    {
        assert(m_queue);
        m_queue->Enqueue(cmd); 
    }
    void Dequeue() 
    {
        assert(m_queue);
        m_queue->Dequeue();
    }
    std::string Front() 
    {
        assert(m_queue);
        return m_queue->Front();
    }
    bool IsEmpty() 
    {
        assert(m_queue);
        return m_queue->IsEmpty();
    }
private:
    std::shared_ptr<CmdQueue> m_queue;
};


} // namespace ftpclient

#endif // _FTP_CMD_QUEUE_H
