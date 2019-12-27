#include "file_save_policy.h"
#include <cassert>

namespace ftpclient {

HANDLE SimpleFileSave::OpenFile(const FTPClientPathType &file)
{
    assert(m_filename.empty());
#if defined(USE_CHAR_PATH)    
    auto fileHandle = ::CreateFileA(file.c_str(),                // name of the write
                      GENERIC_WRITE,          // open for writing
                      FILE_SHARE_READ,         
                      NULL,                   // default security
                      CREATE_NEW,             // create new file only
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                      NULL);
#elif defined(USE_WCHAR_PATH)
    auto fileHandle = ::CreateFileW(file.c_str(),                // name of the write
                      GENERIC_WRITE,          // open for writing
                      FILE_SHARE_READ,         
                      NULL,                   // default security
                      CREATE_NEW,             // create new file only
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,  // normal file
                      NULL);
#else 
#error "unknown type of path"
#endif
    
    assert(fileHandle != INVALID_HANDLE_VALUE);
    return fileHandle;    
}

void SimpleFileSave::SaveFileOnSuccess()
{
    
}
void SimpleFileSave::SaveFileOnFailure()
{
#if defined(USE_CHAR_PATH)    
    ::DeleteFileA(m_filename.c_str());
#elif defined(USE_WCHAR_PATH)
    ::DeleteFileW(m_filename.c_str());
#else 
#error "unknown type of path"
#endif
}

} // end of ftpclient
