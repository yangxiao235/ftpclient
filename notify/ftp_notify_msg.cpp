#include "ftp_notify_msg.h"

namespace ftpclient {

void ReleaseNotify(Notify &notify) 
{
    switch (notify.msg) {
    case MsgEnum::FTP_CMD_NETWORK_CONNECT   : 
    {
        auto dataWithType = Extract<MsgEnum::FTP_CMD_NETWORK_CONNECT>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_CMD_NETWORK_ERROR     :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_CMD_NETWORK_ERROR>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_CMD_NETWORK_CLOSED    :         
    {
        break;
    }
    case MsgEnum::FTP_REPLY                 :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_REPLY>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_DIR_CONTENT           :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_DIR_CONTENT>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_DOWNLOAD_START   :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_DOWNLOAD_START>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS:         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_DOWNLOAD_PROGRESS>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_DOWNLOAD_ERROR   :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_DOWNLOAD_ERROR>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_DOWNLOAD_FINISH  :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_DOWNLOAD_FINISH>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_UPLOAD_START     :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_UPLOAD_START>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_UPLOAD_PROGRESS  :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_UPLOAD_PROGRESS>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_UPLOAD_ERROR     :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_UPLOAD_ERROR>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_FILE_UPLOAD_FINISH    :         
    {
        auto dataWithType = Extract<MsgEnum::FTP_FILE_UPLOAD_FINISH>(notify);
        dataWithType.reset();
        break;
    }
    case MsgEnum::FTP_GET_DIR_CONTENT_ERROR :
    {
        auto dataWithType = Extract<MsgEnum::FTP_GET_DIR_CONTENT_ERROR>(notify);
        dataWithType.reset();
        break;
    }
    default:
        assert(false);
        break;
    }// end of switch (notify.msg)
    notify.data = nullptr;
}


} // namespace ftpclient
