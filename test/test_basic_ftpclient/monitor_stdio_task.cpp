#include "monitor_stdio_task.h"
#include "input_queue.h"
#include <sstream>
#include <regex>

namespace {
void ExtractWord(const std::string &line, std::vector<std::string> &words)
{
    words.clear();
    std::regex pattern(R"(\S+)");
    std::sregex_iterator pos(line.cbegin(), line.cend(), pattern);
    std::sregex_iterator end;
    for (; pos != end; ++pos) {
        words.push_back(pos->str());
    }
}
} // anonymous namespace

namespace ftpclient {

MonitorStdioTask::MonitorStdioTask(CmdQueue::CmdQueueProxy cmdQueue)
    :m_inputQueue(InputQueue::GetInstance()),
    m_cmdQueue(cmdQueue),
    m_notifyQueue(NotifyQueue::GetInstance())
{

}


void MonitorStdioTask::Init() 
{
    
}

void MonitorStdioTask::Run() 
{
    std::string line;
    if (!m_inputQueue.IsEmpty()) {
        line = m_inputQueue.Front();
        m_inputQueue.Dequeue();
        if (line.empty()) {
            return;
        }
        std::vector<std::string> words;
        ExtractWord(line, words);
        if (words.empty()) {
            return;
        }
        auto it = words.cbegin();
        std::ostringstream sout;
        sout << *it++;
        for (; it != words.cend(); ++it) {
            sout << " " << *it;
        }
        sout << "\r\n";
        m_cmdQueue.Enqueue(sout.str());
    }
    Notify notify;;
    if (!m_notifyQueue.IsEmpty()) {
        notify = m_notifyQueue.Front();
        m_notifyQueue.Dequeue();
        switch (notify.msg) {
        case MsgEnum::FTP_CMD_RECV_DATA:
        {
            auto msgdata = ExtractDataOfFTPCmdRecvData(notify);
            notify.data = nullptr;
            fprintf(stderr, msgdata->data.c_str());
            break;
        }
        case MsgEnum::FTP_CMD_NETWORK_CONNECT:
        {
            auto msgdata = ExtractDataOfFTPCmdNetworkConnect(notify);
            notify.data = nullptr;
            fprintf(stderr, "Connection has been established\n");
            break;
        }
        case MsgEnum::FTP_CMD_NETWORK_CLOSED:    
            fprintf(stderr, "Connection has been closed.\n");
            break;
        case MsgEnum::FTP_CMD_NETWORK_ERROR:
        {
            auto msgdata = ExtractDataOfFTPCmdNetworkError(notify);
            fprintf(stderr, "Network error: %s.\n", msgdata->errmsg.c_str());
            break;
        }
        default:
            fprintf(stderr, "Unknown notify type\n");
            ReleaseNotify(notify);
            break;
        }// end of  switch (notify.type) {
    }
}

void MonitorStdioTask::Destroy() noexcept 
{

}

} // namespace ftpclient
