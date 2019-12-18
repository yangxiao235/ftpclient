#ifndef _DATA_TRANS_TASK_H
#define _DATA_TRANS_TASK_H

#include "ftpclient/config.h"
#include "ftpclient/trace.h"
#include "ftpclient/common_types.h"
#include "ftpclient/thread_model/thread_task.h"
#include "ftpclient/thread_model/thread_task_error.h"
#include "data_trans_request.h"
#include "data_trans_request_pending_list.h"
#include "data_trans_request_queue.h"
#include <functional>
#include <cassert>
#include <glog/logging.h>
#include <asio.hpp>

namespace  ftpclient {

class PeriodCheckPendingReqsService : public DataTransRequest
{
public:
    PeriodCheckPendingReqsService(IOContextPointer io, uint16_t period, DataTransRequestPendingList &pendingList)
        :m_period(period),
        m_io(io),
        m_timer(*io),
        m_pendingList(pendingList)
    {}
public:
    void Start() override
    {
        LOG(INFO) << "Period check pending requests service start!";
        SetTimeout();
    }
    bool Complete() override { return false; }
    void Destroy() override 
    { 
        LOG(INFO) << "Period check pending requests service stop!";
        delete this; 
    }
protected:
    void SetTimeout() 
    {
        asio::error_code ec;
        m_timer.expires_from_now(boost::posix_time::seconds(m_period), ec);
        if (ec) {
            LOG(FATAL) << "timer error occured: " << ec.message() << ", error code" << ec.value();
            m_timer.cancel();
            return;
        }
        m_timer.async_wait(std::bind(&PeriodCheckPendingReqsService::Timerhandler, this, std::placeholders::_1));
    }
    void Timerhandler( const asio::error_code& error)
    {
        TRACEA("pending service wakeup again(%d seconds)", m_period);
        if (!error) {
            CheckPendingRequest();
            SetTimeout();
        } else {
            LOG(FATAL) << "timer error occured: " << error.message() << ", error code" << error.value();
            m_timer.cancel();
        }
    }
    void CheckPendingRequest() 
    {
        m_pendingList.RemoveIf( [] (DataTransRequestProxy& req) {
            if (req.Complete()) {
                DLOG(INFO) << "One task has been removed from pending list.";
                return true;
            }
            return false;});
    }
private:
    asio::deadline_timer   m_timer;
    uint16_t         m_period;
    IOContextPointer m_io;
    DataTransRequestPendingList &m_pendingList;
};

class PeriodCheckInputQueueService : public DataTransRequest
{
public:
    PeriodCheckInputQueueService(IOContextPointer io, 
                                             uint16_t period,
                                             DataTransRequestPendingList &pendingList,
                                             DataTransRequestQueueProxy queue)
        :m_io(io),
        m_period(period),
        m_queue(queue),
        m_timer(*io),
        m_pendingList(pendingList)
    {}
public:
    void Start() override
    {
        LOG(INFO) << "Period check input requests service start!";
        SetTimeout();
    }
    bool Complete() override { return false; }
    void Destroy() override 
    {
        LOG(INFO) << "Period check input requests service stop!";
        delete this; 
    }
protected:
    void SetTimeout() 
    {
        asio::error_code ec;
        m_timer.expires_from_now(boost::posix_time::seconds(m_period), ec);
        if (ec) {
            LOG(FATAL) << "timer error occured: " << ec.message() << ", error code" << ec.value();
            m_timer.cancel();
        } else {
            m_timer.async_wait(std::bind(&PeriodCheckInputQueueService::Timerhandler, this, std::placeholders::_1));
        }
    }
    void Timerhandler( const asio::error_code& error)
    {
        TRACEA("input service wakeup again(%d seconds)", m_period);
        if (!error) {
            CheckRequestQueue();
            SetTimeout();
        } else {
            LOG(FATAL) << "timer error occured: " << error.message() << ", error code" << error.value();
            m_timer.cancel();
        }
    }
    void CheckRequestQueue() 
    {
        if (!m_queue.IsEmpty()) {
            auto req = m_queue.Front();
            m_queue.Dequeue();
            req.Start();
            m_pendingList.PushBack(req);
        }
    }
private:
    asio::deadline_timer   m_timer;
    uint16_t  m_period;
    IOContextPointer m_io;
    DataTransRequestQueueProxy  m_queue;
    DataTransRequestPendingList &m_pendingList;
};

    
class DataTransTask : public ThreadTask
{
public:
    DataTransTask(IOContextPointer      io, DataTransRequestQueueProxy queue)
        : m_io(io),
        m_queue(queue)
    {
        assert(m_io);
    }
    ~DataTransTask() = default;    
    void Init() override
    {
        // 创建监视服务
        auto pendingCheckService = DataTransRequestProxy::Create(new PeriodCheckPendingReqsService(m_io, 5, m_list));
        auto inputQueueCheckService = DataTransRequestProxy::Create(new PeriodCheckInputQueueService  (m_io, 2, m_list, m_queue));
        pendingCheckService.Start();
        inputQueueCheckService.Start();
        m_list.PushBack(pendingCheckService);
        m_list.PushBack(inputQueueCheckService);
    }
    void Run()  override
    {
        try {
          m_io->poll();        
        } catch(const asio::system_error &ex) {
            LOG(FATAL) << "ASIO exception: " << ex.what() << ", error code " << ex.code().value();
            throw TaskRunFailed("asio exception");
        }  
    }
    void Destroy() noexcept override
    {
        delete this;
    }
private:
    // 所有request的共享io
    IOContextPointer  m_io;
    DataTransRequestPendingList m_list;
    // 线程共享
    DataTransRequestQueueProxy  m_queue;
};


} // namespace ftpclient

#endif // _DATA_TRANS_TASK_H
