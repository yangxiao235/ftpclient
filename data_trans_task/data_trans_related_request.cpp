#include "data_trans_related_request.h"
#include <windows.h>
#include <system_error>

namespace ftpclient {

SimpleFileSavePolicy<std::string>::SimpleFileSavePolicy(IOContextPointer io)
    :m_handle(*io)
{

}

SimpleFileSavePolicy<std::string>::~SimpleFileSavePolicy()
{

}

void SimpleFileSavePolicy<std::string>::OpenFile(const std::string &file, std::error_code &ec)
{
    ec.clear();
    auto fileHandle = ::CreateFileA(file.c_str(),                // name of the write
                      GENERIC_WRITE,          // open for writing
                      0,         
                      NULL,                   // default security
                      OPEN_ALWAYS,             // create new file only
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                      NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        ec.assign(static_cast<int>(::GetLastError()), asio::system_category());        
        return;
    }    
    m_handle.assign(fileHandle);
    fileHandle = INVALID_HANDLE_VALUE;
}


void SimpleFileSavePolicy<std::string>::Cancel()
{
    m_handle.cancel();
}

void SimpleFileSavePolicy<std::string>::CloseOnComplete()
    {
    m_handle.close();
}

void SimpleFileSavePolicy<std::string>::CloseOnFailure()
{
    m_handle.close();
}

} // namespace ftpclient
