#ifndef _FILE_SAVE_POLICY_H
#define _FILE_SAVE_POLICY_H

#include "ftpclient/platform_specific.h"
#include <windows.h>

namespace ftpclient {

class SimpleFileSave
{
public:
    HANDLE OpenFile(const FTPClientPathType &file);
    // 写入已完成完成, 实现不同的文件保存策略
    void SaveFileOnSuccess();
    // 文件写入过程发生错误或者准备数据过程出现错误导致文件数据不完整时,
    // 使用此调用. 另外, 多次调用之应该无伤
    void SaveFileOnFailure();
private:
    std::string m_filename;
};


//template <class Path>
//class TemporaryFileSave
//{
//public:
//    HANDLE OpenFile(const Path &file);
//    // 写入已完成完成, 实现不同的文件保存策略
//    void SaveFileOnSuccess();
//    // 文件写入过程发生错误或者准备数据过程出现错误导致文件数据不完整时,
//    // 使用此调用. 另外, 多次调用之应该无伤
//    void SaveFileOnFailure();
//};


} // namesapce ftpclient
#endif // _FILE_SAVE_POLICY_H