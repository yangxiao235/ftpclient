#ifndef _FTP_CMD_H
#define _FTP_CMD_H

#include "notify_msg.h"

namespace ftpclient {
using FTPCmdGroupPointer = std::shared_ptr<FTPCmd>;
struct FTPCmd 
{
    bool        control;
    std::string data;
};    

class FTPCmdGroup
{
public:
    enum Status {NOTFINISH, NEXT, FINISH};
    virtual ~FTPCmdGroup() {}
    using const_iterator = std::vector<FTPCmd>::const_iterator;
    const_iterator cbegin() { return m_cmdList.cbegin(); }
    const_iterator cend() { return m_cmdList.cend(); }
    virtual Status OnRecieve(const Reply&) { return true; }
protected:
    FTPCmdGroup();    
protected:
    static bool IsControlCmd(const std::string &cmd)
    {
        return kControlCmds.find(cmd) != kControlCmds.end();
    }
    static bool IsModelACmd(const std::string &cmd)
    {
        return kModelACmds.find(cmd) != kModelACmds.end();
    }
    static bool IsModelBCmd(const std::string &cmd)
    {
        return kModelBCmds.find(cmd) != kModelBCmds.end();
    }        
    static const std::set<std::string> kControlCmds; 
    static const std::set<std::string> kModelACmds;             
    static const std::set<std::string> kModelBCmds;     
protected:
    std::vector<FTPCmd> m_cmdList;  
};


class FTPOneCmdGroupModelA : public FTPCmdGroup
{
public:
    template <typename... Args>
    FTPOneCmdGroupModelA(const std::string &cmd, const Args&... args)
    {
        assert(IsModelACmd(cmd));
        FTPCmd cmdinfo;
        cmdinfo.control = IsControlCmd(cmd);
        cmdinfo.data = BuildCmd(cmd, args);
        m_cmdList.push_back(m_cmdList);
    }    
    bool OnRecieve(const Reply&) override;
private:
    enum  State { 
        START,
        COMPLETE,
        ERROR
    };
    State m_state;   
};

class FTPOneCmdGroupModelB : public FTPCmdGroup
{
public:
    template <typename... Args>
    FTPOneCmdGroupModelB(const std::string &cmd, const Args&... args)
    {
        assert(IsModelBCmd(cmd));
        FTPCmd cmdinfo;
        cmdinfo.control = IsControlCmd(cmd);
        cmdinfo.data = BuildCmd(cmd, args);
        m_cmdList.push_back(m_cmdList);
    }    
    bool OnRecieve(const Reply&) override;
private:
    enum  State { 
        START,
        CONTINUE,    
        COMPLETE,
        ERROR
    };
    State m_state;   
};



class Login : public FTPCmdGroup  
{
public:
    Login(const std::string &name, const std::string &password)
    {
        m_cmdList.push_back({BuildCmd("USER", name), false});
        m_cmdList.push_back({BuildCmd("PASS", password), false});        
    }
    bool OnRecieve(const Reply&) override;
private:    
    enum  State { 
        START, 
        FIRST_STAGE, 
        COMPLETE,
        ERROR
        };
    State  m_state = START;
};

class RenameFile : public FTPCmdGroup  
{
public:    
    RenameFile(const std::string &src, const std::string &target)
    {
        m_cmdList.push_back({BuildCmd("RNFR", src), false});
        m_cmdList.push_back({BuildCmd("RNTO", target), false});        
    }
    bool OnRecieve(const Reply&) override;
private:
    enum  State { 
        START, 
        FIRST_STAGE, 
        COMPLETE,
        ERROR
        };
    State  m_state = START;
};



} // namepsace ftpclient

#endif // _FTP_CMD_H