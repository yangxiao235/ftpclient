#ifndef _FILE_SAVE_POLICY_H
#define _FILE_SAVE_POLICY_H

#include "ftpclient/platform_specific.h"
#include <windows.h>

namespace ftpclient {

class SimpleFileSave
{
public:
    HANDLE OpenFile(const FTPClientPathType &file);
    // д����������, ʵ�ֲ�ͬ���ļ��������
    void SaveFileOnSuccess();
    // �ļ�д����̷����������׼�����ݹ��̳��ִ������ļ����ݲ�����ʱ,
    // ʹ�ô˵���. ����, ��ε���֮Ӧ������
    void SaveFileOnFailure();
private:
    std::string m_filename;
};


//template <class Path>
//class TemporaryFileSave
//{
//public:
//    HANDLE OpenFile(const Path &file);
//    // д����������, ʵ�ֲ�ͬ���ļ��������
//    void SaveFileOnSuccess();
//    // �ļ�д����̷����������׼�����ݹ��̳��ִ������ļ����ݲ�����ʱ,
//    // ʹ�ô˵���. ����, ��ε���֮Ӧ������
//    void SaveFileOnFailure();
//};


} // namesapce ftpclient
#endif // _FILE_SAVE_POLICY_H