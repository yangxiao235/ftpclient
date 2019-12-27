#ifndef _FTP_NOTIFY_POLICY_H
#define _FTP_NOTIFY_POLICY_H

#include "ftp_notify_queue.h"
#include <cstdio>

namespace ftpclient {

class NotifyUsingQueue
{
public:
    static void ReportEvent(const Notify &notify)
    {
        NotifyQueue::GetInstance().Enqueue(notify);
    }

    static void ReportCmdNetworkConnect(const std::string &peerIP, uint16_t peerPort)
    {
        
        NotifyQueue::GetInstance().Enqueue(BuildCmdNetworkConnect(peerIP, peerPort));
    }
    static void ReportCmdNetworkError(const std::string &errmsg)
    {

        NotifyQueue::GetInstance().Enqueue(BuildCmdNetworkError(errmsg));
    }
    static void ReportCmdNetworkClosed()
    {
        NotifyQueue::GetInstance().Enqueue(BuildCmdNetworkClosed());
    }
    static void ReportFTPReply(const DataOfFTPReply &reply)
    {
        NotifyQueue::GetInstance().Enqueue(BuildFTPReply(reply));
    }

    static void ReportFTPCmdRecvData(const std::string &data)
    {
        NotifyQueue::GetInstance().Enqueue(BuildCmdRecvData(data));       
    }
    static void ReportDirContent(const std::string &dir, std::unique_ptr<char[]> buf, size_t bufSize)
    {
        NotifyQueue::GetInstance().Enqueue(BuildDirContent(dir, std::move(buf), bufSize));
    }
    static void ReportDownloadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        NotifyQueue::GetInstance().Enqueue(BuildDownloadStart(filenameOnSever, filenameOnLocal));
    }
    static void ReportDownloadProgress(const std::string &filenameOnSever, 
                                                  const FTPClientPathType &filenameOnLocal,
                                                  uint64_t bytesRecv,
                                                  uint64_t fileSize)
    {
        NotifyQueue::GetInstance().Enqueue(BuildDownloadProgress(filenameOnSever, filenameOnLocal, bytesRecv, fileSize));
    }
    static void ReportDownloadError(const std::string &filenameOnSever, 
                                            const FTPClientPathType &filenameOnLocal,
                                            const std::string &errmsg)
    {
        NotifyQueue::GetInstance().Enqueue(BuildDownloadError(filenameOnSever, filenameOnLocal, errmsg));
    }
    static void ReportDownloadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        NotifyQueue::GetInstance().Enqueue(BuildDownloadFinish(filenameOnSever, filenameOnLocal));
    }
    
    static void ReportUploadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        NotifyQueue::GetInstance().Enqueue(BuildUploadStart(filenameOnSever, filenameOnLocal));
    }
    static void ReportUploadProgress(const std::string &filenameOnSever, 
                                                  const FTPClientPathType &filenameOnLocal,
                                                  uint64_t byteSend,
                                                  uint64_t fileSize)
    {
        NotifyQueue::GetInstance().Enqueue(BuildUploadProgress(filenameOnSever, filenameOnLocal, byteSend, fileSize));
    }
    static void ReportUploadError(const std::string &filenameOnSever, 
                                            const FTPClientPathType &filenameOnLocal,
                                            const std::string &errmsg)
    {
        NotifyQueue::GetInstance().Enqueue(BuildUploadError(filenameOnSever, filenameOnLocal, errmsg));
    }
    static void ReportUploadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        NotifyQueue::GetInstance().Enqueue(BuildUploadFinish(filenameOnSever, filenameOnLocal));
    }
};

//class NotifyUsingWinPostMessageAPI
//{
//public:
//    static void ReportEvent(const Notify &notify)
//    {
//    }
//
//    static void ReportCmdNetworkConnect(const std::string &peerIP, uint16_t peerPort)
//    {
//        
//    }
//    static void ReportCmdNetworkError(const std::string &errmsg)
//    {
//    }
//    static void ReportCmdNetworkClosed()
//    {
//    }
//    static void ReportFTPReply(const DataOfFTPReply &reply)
//    {
//    }
//    static void ReportDirContent(const std::string &dir, std::unique_ptr<char[]> buf, size_t bufSize)
//    {
//    }
//    static void ReportDownloadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
//    {
//    }
//    static void ReportDownloadProgress(const std::string &filenameOnSever, 
//                                                  const FTPClientPathType &filenameOnLocal,
//                                                  uint64_t bytesRecv,
//                                                  uint64_t fileSize)
//    {
//    }
//    static void ReportDownloadError(const std::string &filenameOnSever, 
//                                            const FTPClientPathType &filenameOnLocal,
//                                            const std::string &errmsg)
//    {
//    }
//    static void ReportDownloadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
//    {
//    }
//    
//    static void ReportUploadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
//    {
//    }
//    static void ReportUploadProgress(const std::string &filenameOnSever, 
//                                                  const FTPClientPathType &filenameOnLocal,
//                                                  uint64_t byteSend,
//                                                  uint64_t fileSize)
//    {
//    }
//    static void ReportUploadError(const std::string &filenameOnSever, 
//                                            const FTPClientPathType &filenameOnLocal,
//                                            const std::string &errmsg)
//    {
//    }
//    static void ReportUploadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
//    {
//    }    
//
//};


class NotifyPolicyForTest
{
public:
    static void ReportEvent(const Notify &notify)
    {
        fprintf(stderr, "unknown msg, msg code %d\n", notify.msg);
        fprintf(stderr, "------------------------\n");
    }

    static void ReportCmdNetworkConnect(const std::string &peerIP, uint16_t peerPort)
    {
        fprintf(stderr, "Msg CmdNetworkConnect\n");
        fprintf(stderr, "------------\n");
        fprintf(stderr, "ip %s, port %d\n", peerIP.c_str(), peerPort);
    }
    static void ReportCmdNetworkError(const std::string &errmsg)
    {
        fprintf(stderr, "Msg CmdNetworkError\n");
        fprintf(stderr, "------------\n");
        fprintf(stderr, "%s\n", errmsg.c_str());
    }
    static void ReportCmdNetworkClosed()
    {
        fprintf(stderr, "Msg CmdNetworkClosed\n");
        fprintf(stderr, "--------------------\n");
    }
    static void ReportFTPReply(const DataOfFTPReply &reply)
    {
        fprintf(stderr, "Msg FTPReply\n");
        fprintf(stderr, "------------\n");
        fprintf(stderr, "%s\n", reply.detail.c_str());
    }
    static void ReportDirContent(const std::string &dir, std::unique_ptr<char[]> buf, size_t bufSize)
    {
        fprintf(stderr, "Msg DirContent\n");
        fprintf(stderr, "--------------\n");
        buf[bufSize] = 0;
        fprintf(stderr, "%s\n", buf.get());
    }
    static void ReportDownloadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        fprintf(stderr, "Msg DownloadStart\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnSever.c_str(), filenameOnLocal.c_str());
    }
    static void ReportDownloadProgress(const std::string &filenameOnSever, 
                                                  const FTPClientPathType &filenameOnLocal,
                                                  uint64_t bytesRecv,
                                                  uint64_t fileSize)
    {
        fprintf(stderr, "Msg DownloadProgress\n");
        fprintf(stderr, "--------------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnSever.c_str(), filenameOnLocal.c_str());
        fprintf(stderr, "total: %lld, accomplished: %lld\n", fileSize, bytesRecv);
    }
    static void ReportDownloadError(const std::string &filenameOnSever, 
                                            const FTPClientPathType &filenameOnLocal,
                                            const std::string &errmsg)
    {
        fprintf(stderr, "Msg DownloadError\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnSever.c_str(), filenameOnLocal.c_str());
        fprintf(stderr, "%s\n", errmsg.c_str());
    }
    static void ReportDownloadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        fprintf(stderr, "Msg DownloadFinish\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnSever.c_str(), filenameOnLocal.c_str());
    }
    
    static void ReportUploadStart(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        fprintf(stderr, "Msg UploadStart\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnLocal.c_str(), filenameOnSever.c_str());
    }
    static void ReportUploadProgress(const std::string &filenameOnSever, 
                                                  const FTPClientPathType &filenameOnLocal,
                                                  uint64_t byteSend,
                                                  uint64_t fileSize)
    {
        fprintf(stderr, "Msg UploadProgress\n");
        fprintf(stderr, "--------------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnLocal.c_str(), filenameOnSever.c_str());
        fprintf(stderr, "total: %lld, accomplished: %lld\n", fileSize, byteSend);
    }
    static void ReportUploadError(const std::string &filenameOnSever, 
                                            const FTPClientPathType &filenameOnLocal,
                                            const std::string &errmsg)
    {
        fprintf(stderr, "Msg UploadError\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnLocal.c_str(), filenameOnSever.c_str());
        fprintf(stderr, "%s\n", errmsg.c_str());
    }
    static void ReportUploadFinish(const std::string &filenameOnSever, const FTPClientPathType &filenameOnLocal)
    {
        fprintf(stderr, "Msg UploadFinish\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "src: %s, dest: %s\n", filenameOnLocal.c_str(), filenameOnSever.c_str());
    }    

    static void ReportGetDirContentError(const std::string &errmsg)
    {
        fprintf(stderr, "Msg GetDirContentError\n");
        fprintf(stderr, "--------------\n");
        fprintf(stderr, "%s\n", errmsg.c_str());
    }
};

} // namespace ftpclient
#endif // _FTP_NOTIFY_POLICY_H
