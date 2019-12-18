#ifndef _FTP_NOTIFY_QUEUE_H
#define _FTP_NOTIFY_QUEUE_H

#include "ftpclient/ftp.h"
#include <queue>
#include <mutex>

namespace ftpclient
{
enum class MsgType 
{
    DEFAULT = 0,
    NETWORK_CONNECT,
    NETWORK_ERROR,
    NETWORK_CLOSED,
    FTP_REPLY,
    FTP_DIR_CONTENT
};
    
struct Msg {
    std::string data;
};

struct Notify
{
    MsgType type;
    union {  
        Msg msg;
        Reply reply;
        DirContent content;
    };
    Notify(MsgType type_)
        :type(type_)
    {
        if (type == MsgType::FTP_REPLY) {
            new (&reply) Reply();
        } else if (type == MsgType::FTP_DIR_CONTENT) {
            new (&content) DirContent();
        } else {
            new (&msg) Msg();
        }
    }
    
    Notify(const Notify &rhs) 
    {
        if (this != &rhs) {
            type = rhs.type;
            if (type == MsgType::FTP_REPLY) {
                new (&reply) Reply(rhs.reply);
            } else if (type == MsgType::FTP_DIR_CONTENT) {
                new (&content) DirContent(rhs.content);
            } else {
                new (&msg) Msg();
            }
        }
    }
    Notify &operator=(const Notify &rhs)
    {
        if (this != &rhs) {
            // 释放原来的空间
            if (type == MsgType::FTP_REPLY) {
                reply.~Reply();
            } else if (type == MsgType::FTP_DIR_CONTENT) {
                content.~DirContent();
            } else {
                msg.~Msg();
            }
            type = rhs.type;
            if (type == MsgType::FTP_REPLY) {
                new (&reply) Reply(rhs.reply);
            } else if (type == MsgType::FTP_DIR_CONTENT) {
                new (&content) DirContent(rhs.content);
            } else {
                new (&msg) Msg();
            }
        }
        return *this;
    }    
    ~Notify() {
        if (type == MsgType::FTP_REPLY) {
            reply.~Reply();
        } else if (type == MsgType::FTP_DIR_CONTENT) {
            content.~DirContent();
        } else {
            msg.~Msg();
        }
    }
};

class NotifyQueue
{
public:
    static NotifyQueue & GetInstance();
public:
    void Enqueue(const Notify &);
    void Dequeue();
    Notify Front();
    bool IsEmpty();
    ~NotifyQueue() = default;
private:    
    NotifyQueue() = default;
    NotifyQueue(const NotifyQueue&) = delete;
    NotifyQueue &operator=(const NotifyQueue&) = delete;
    
    std::mutex  m_accessMutex;
    std::queue<Notify> m_queue; 
};


}// namespace ftpclient


#endif // _FTP_NOTIFY_QUEUE_H
