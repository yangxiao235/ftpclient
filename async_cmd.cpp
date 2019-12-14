#include "async_cmd.h"

namespace ftpclient {

AsyncCmd::AsyncCmdProxy AsyncCmd::Create(AsyncCmd *cmd)
{
    AsyncCmdProxy cmdProxy{cmd};
    return cmdProxy;
}

} // namespace ftpclient
