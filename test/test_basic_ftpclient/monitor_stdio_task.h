#include "ftpclient/thread_model/thread_task.h"
#include "ftpclient/cmd_task/ftp_cmd_queue.h"
#include "ftpclient/notify/ftp_notify_queue.h"
#include "input_queue.h"

namespace ftpclient {

class MonitorStdioTask : public ThreadTask
{
public:
    MonitorStdioTask(CmdQueue::CmdQueueProxy cmdQueue);
protected:
    ~MonitorStdioTask() = default;
public:
    void Init() override;
    void Run() override;
    void Destroy() noexcept override;
private:
    InputQueue &m_inputQueue;
    CmdQueue::CmdQueueProxy m_cmdQueue;
    NotifyQueue &m_notifyQueue;
};


} // namespace ftpclient
