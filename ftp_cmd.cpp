#include "ftp_cmd.h"
namespace ftpclient {

const std::set<std::string> FTPCmdGroup::kControlCmds = 
    {  "ABOR", "QUIT", "STAT" }; 
const std::set<std::string> FTPCmdGroup::kModelACmds = 
{
    "ABOR","ALLO","DELE","CWD","CDUP","SMNT","HELP","MODE","NOOP","PASV",
    "QUIT","SITE","PORT","SYST","STAT","RMD","MKD","PWD","STRU","TYPE"
};             
const std::set<std::string> FTPCmdGroup::kModelBCmds = 
{ "APPE","LIST","NLIST","REIN","RETR","STOR","STOU" };  

bool FTPOneCmdGroupModelA::OnRecieve(const Reply &reply)
{
    char code = reply.first[0];
    switch (m_state) {
    case START:
        if (code == '1' ||
            code == '3' ||
            code == '4' ||
            code == '5') {
            m_state = ERROR;
        } else if (code == '2') {
            m_state = COMPLETE;
        } else {
            LOG(FATAL) << "FTPOneCmdGroupModelA::OnRecieve(): invalid ftp reply \'" << reply.first << "\'";
        }
        break;
    case COMPLETE:
        break;
    default:
        break;
    }    
    if (m_state == COMPLETE) {
        NotifyMsg msg;
        msg.type = MsgType::FTP_CMD_OK;
        msg.data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else if (m_state == ERROR) {
        NotifyMsg msg;
        msg.type = MsgType::FTP_CMD_FAILED;
        msg.data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else {
        return false;
    }
}

bool FTPOneCmdGroupModelB::OnRecieve(const Reply &reply)
{
    switch (m_state) {
    case START:
        if (reply == '3' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else if (reply == '2') {
            m_state = COMPLETE;
        } else if (reply == '1') {
            m_state = CONTINUE; 
        } else {
            // 非法�?
        }
        break;
    case CONTINUE:
        if (reply == '3' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else if (reply == '2') {
            m_state = COMPLETE;
        } else {
            
        }
        break;
    case COMPLETE:
        break;
    default:
        break;    
    } // end of swtich()
   if (m_state == COMPLETE) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = true;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else if (m_state == ERROR) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = false;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else {
        return false;
    }    
}

bool Login::OnRecieve(const Reply &reply)
{
 switch (m_state) {
    case START:
        if (reply == '1' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else if (reply == '2') {
            m_state = COMPLETE;
        } else { 
            // reply == '3''
            m_state = FIRST_STAGE;
        }
        break;
    case FIRST_STAGE:
        if (reply == '1' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else if (reply == '3') {
            // TODO: ACCT 功能不支�?
            // 可在日志文件中记�?
            m_state = ERROR;
        } else {
            // reply == '2'
            m_state = COMPLETE;
        }
        break;
    case COMPLETE:
        // 命令一旦完�? 不应该再次调用OnRecieveReply()
        break;
    case ERROR:
        // 命令一旦完�? 不应该再次调用OnRecieveReply()
        break;
    default:
        break;
    }  // end of switch
   if (m_state == COMPLETE) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = true;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else if (m_state == ERROR) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = false;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else {
        return false;
    }      
}

bool RenameFile::OnRecieve(const Reply &reply)
{
    switch (m_state) {
    case START:
        if (reply == '1' ||
            reply == '2' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else { 
            // reply == '3''
            m_state = FIRST_STAGE;
        }
        break;
    case FIRST_STAGE:
        if (reply == '1' ||
            reply == '3' ||
            reply == '4' ||
            reply == '5') {
            m_state = ERROR;
        } else {
            // reply == '2'
            m_state = COMPLETE;
        }
        break;
    case COMPLETE:
        // 命令一旦完�? 不应该再次调用OnRecieveReply()
        break;
    case ERROR:
        // 命令一旦完�? 不应该再次调用OnRecieveReply()
        break;
    default:
        break;
    }    // end of swtich
   if (m_state == COMPLETE) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = true;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else if (m_state == ERROR) {
        NotifyMsg msg;
        msg.type = m_msgType;
        auto data = new FTPCmdNotify;
        data->success = false;
        data->reply = reply;
        msg.data = reinterpret_cast<void *>(data);
        data = nullptr;
        NotifyQueue::GetInstance().EnqueueMsg(msg);
        return true;
    } else {
        return false;
    }      
}


} // namspace ftpclient