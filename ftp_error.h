#ifndef _CMD_ERROR_H
#define _CMD_ERROR_H

#include <string>
#include <system_error>
#include <mutex>

namespace ftpclient {

enum class FTPError
{
    NETWORK_ERROR = 1,
    FTP_REQUEST_REFUSED,
};


class FTPErrorCategory : public std::error_category
{
public:
    static const FTPErrorCategory &GetInstance()
    {
        static FTPErrorCategory *instance = nullptr;
        if (!instance) {
            static std::mutex  mx;
            std::lock_guard<std::mutex> lock(mx);
            if (!instance) {
                instance = new FTPErrorCategory();
            }
        }
        return *instance;
    }

    const char *name() const noexcept override
    {
        return "ftpclient.error";
    }
    std::string message(int condition) const override
    {
        switch (condition) {
        case static_cast<int>(FTPError::NETWORK_ERROR):
            return "network error";
        case static_cast<int>(FTPError::FTP_REQUEST_REFUSED):
            return "ftp service request refused";
        }
        return "ftpclient unknown error";
    }
};

} // namespace ftpclient
#endif // _CMD_ERROR_H
