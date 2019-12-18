#ifndef _DATA_TRANS_REQUEST_PENDING_QUEUE_H
#define _DATA_TRANS_REQUEST_PENDING_QUEUE_H

#include "data_trans_request.h"
#include <memory>
#include <queue>
#include <mutex>
#include <cassert>
#include <list>
#include <functional>

namespace ftpclient {

class DataTransRequestPendingList
{
public:
    using Iterator = std::list<DataTransRequestProxy>::iterator;
public:
    void PushBack(DataTransRequestProxy req)
    {
        m_list.push_back(req);
    }
    Iterator begin()
    {
        return m_list.begin();
    }
    Iterator end()
    {
        return m_list.end();
    }
    void Remove(Iterator pos)
    {
        m_list.erase(pos);
    }

    void RemoveIf(std::function<bool (DataTransRequestProxy &)> &&checker)
    {
        m_list.remove_if(checker);
    }
private:
    std::list<DataTransRequestProxy> m_list;  
};

} // namespace ftpclient

#endif // _DATA_TRANS_REQUEST_PENDING_QUEUE_H

