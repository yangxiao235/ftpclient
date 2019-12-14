#ifndef _ASYNC_CMD_H
#define _ASYNC_CMD_H

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <system_error>
#include <utility>
#include <cassert>

namespace ftpclient {

struct Notify;

using Handler = std::function<void(const std::error_code &)>;
class AsyncCmd 
{
public:
    class AsyncCmdProxy;
    static AsyncCmdProxy Create(AsyncCmd *);
    virtual ~AsyncCmd() = default;
    virtual void Start() = 0;
    virtual bool OnRecieved(const Notify &, std::error_code &) = 0;
protected:
    AsyncCmd() = default;
};

class AsyncCmd::AsyncCmdProxy
{
public:
    AsyncCmdProxy() = default;
    AsyncCmdProxy(const AsyncCmdProxy &) = default;
    AsyncCmdProxy &operator=(const AsyncCmdProxy &) = default;
    ~AsyncCmdProxy() = default;

    void Start()
    {
        assert(m_cmd);
        m_cmd->Start();
    }
    bool OnRecieved(const Notify &notify, std::error_code &ec) 
    {
        assert(m_cmd);
        return m_cmd->OnRecieved(notify, ec);
    }
protected:
    friend  AsyncCmdProxy AsyncCmd::Create(AsyncCmd *);
    AsyncCmdProxy(AsyncCmd *cmd)
        :m_cmd(cmd)
    {
        assert(m_cmd);
    }    
private:
    std::shared_ptr<AsyncCmd> m_cmd;
};

using CmdAndHandlerPair = std::pair<AsyncCmd::AsyncCmdProxy, Handler>;

} // namespace ftpclient

#endif // _ASYNC_CMD_H
