#include "ftpclient/notify/ftp_notify_msg.h"
#include <string>
#include <iostream>

using namespace ftpclient;

int main()
{
    {
        Notify notify;
        auto data = new DataOfFTPFileUploadProgress;
        data->filenameOnLocal = "D:\\doc\\somedoc.txt";
        data->filenameOnServer = "/home/ftp/doc/somedoc.txt";
        data->bytesSend = 3000;
        data->fileSize = 8091;
        notify.msg = MsgEnum::FTP_FILE_UPLOAD_PROGRESS;
        notify.data = reinterpret_cast<void *>(data);

        auto msgdata = ExtractDataOfFTPFileUploadProgress(notify);
        std::cout << msgdata->filenameOnLocal << std::endl;
        std::cout << msgdata->filenameOnServer << std::endl;
        std::cout << msgdata->bytesSend << std::endl;
        std::cout << msgdata->fileSize << std::endl;
    }
    std::cout << "-----------------" <<std::endl;
    {
        Notify notify;
        auto data = new DataOfFTPFileDownloadFinish;
        data->filenameOnServer = "/home/ftp/doc/somedoc.txt";
        data->filenameOnLocal = "D:\\temp\\doc.txt";
        notify.msg = MsgEnum::FTP_FILE_DOWNLOAD_FINISH;
        notify.data = reinterpret_cast<void *>(data);
        
        auto msgdata = ExtractDataOfFTPFileDownloadFinish(notify);
        std::cout << msgdata->filenameOnLocal << std::endl;
        std::cout << msgdata->filenameOnServer << std::endl;
    }
    std::cout << "-----------------" <<std::endl;
    {
        Notify notify;
        auto data = new DataOfFTPCmdNetworkConnect;
        data->peerIP = "127.0.0.1";
        data->peerPort = 9055;
        notify.msg = MsgEnum::FTP_CMD_NETWORK_CONNECT;
        notify.data = reinterpret_cast<void *>(data);
        
        auto msgdata = ExtractDataOfFTPCmdNetworkConnect(notify);
        std::cout << msgdata->peerIP << std::endl;
        std::cout << msgdata->peerPort << std::endl;
    }
        
}
