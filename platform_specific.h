#ifndef _PLATFORM_SPECIFIC_H
#define _PLATFORM_SPECIFIC_H

#include "ftpclient/config.h"
#include <string>

#if defined(USE_WCHAR_PATH)
#define FTPClientPathType  std::wstring
#elif defined(USE_CHAR_PATH)
#define FTPClientPathType  std::string
#else 
#error "unknown path type definition!"
#endif


#endif 
