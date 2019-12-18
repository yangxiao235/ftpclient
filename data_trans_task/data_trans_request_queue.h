#ifndef _DATA_TRANS_REQUEST_QUEUE_H
#define _DATA_TRANS_REQUEST_QUEUE_H

#include "data_trans_request.h"
#include <memory>
#include <queue>
#include <mutex>
#include <cassert>

namespace ftpclient {

class DataTransRequestQueue
{
public:
    ~DataTransRequestQueue() = default;
protected:
    DataTransRequestQueue() = default;
public:
    void Enqueue(const DataTransRequestProxy &req)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(req);
    }

    void Dequeue()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        assert(!m_queue.empty());
        m_queue.pop();
    }

    DataTransRequestProxy Front()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        assert(!m_queue.empty());
        return m_queue.front();
    }

    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
private:
    friend class DataTransRequestQueueProxy;
    std::mutex  m_mutex;    
    std::queue<DataTransRequestProxy> m_queue;  
};

class DataTransRequestQueueProxy
{
public:
    static DataTransRequestQueueProxy Create()
    {
        DataTransRequestQueueProxy queueProxy;
        queueProxy.m_queue.reset(new DataTransRequestQueue);
        return queueProxy;
    }
public:
    void Enqueue(const DataTransRequestProxy &req)
    {
        m_queue->Enqueue(req);
    }

    void Dequeue()
    {
        m_queue->Dequeue();
    }

    DataTransRequestProxy Front()
    {
        return m_queue->Front();
    }

    bool IsEmpty()
    {
        return m_queue->IsEmpty();
    }
private:
    std::shared_ptr<DataTransRequestQueue> m_queue;
};

} // namespace ftpclient


#endif // _DATA_TRANS_REQUEST_QUEUE_H
