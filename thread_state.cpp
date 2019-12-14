#include "thread_state.h"

namespace ftpclient 
{
ThreadState::ThreadStateProxy ThreadState::Create()
{
    return ThreadStateProxy(new ThreadState);
}

}
