#ifndef _DATA_TRANS_REQUEST_H
#define _DATA_TRANS_REQUEST_H

#include <memory>
#include <cassert>

namespace ftpclient {

class DataTransRequest
{
public:
    virtual ~DataTransRequest() {}
protected:
    DataTransRequest() = default;    
public:
    virtual void Start() = 0; 
    virtual bool Complete() = 0;
    virtual void Destroy() = 0;
};

class DataTransRequestProxy
{
public:
    DataTransRequestProxy() = default;
    static DataTransRequestProxy Create(DataTransRequest *req)
    {
        return DataTransRequestProxy{req};
    }
protected:
    DataTransRequestProxy(DataTransRequest *request)
        :m_request(std::shared_ptr<DataTransRequest>(request, [](DataTransRequest *ptr) { if (ptr) ptr->Destroy(); }))
    {
        assert(m_request);
    }
public:
    void Start() 
    {
        assert(m_request);
        m_request->Start();
    }
    bool Complete()
    {
        assert(m_request);
        return m_request->Complete();
    }
private:
    std::shared_ptr<DataTransRequest> m_request;
};

} // namespace ftpclient


#endif // _DATA_TRANS_REQUEST_H
