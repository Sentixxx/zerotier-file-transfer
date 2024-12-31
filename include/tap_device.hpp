#ifndef TAP_DEVICE_HPP
#define TAP_DEVICE_HPP

#include <string>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

class TapDevice {
public:
    explicit TapDevice(const std::string& dev_name = "tapyuan");
    ~TapDevice();

    // 禁用拷贝
    TapDevice(const TapDevice&) = delete;
    TapDevice& operator=(const TapDevice&) = delete;
    
    // 允许移动
    TapDevice(TapDevice&&) noexcept;
    TapDevice& operator=(TapDevice&&) noexcept;

    int getFd() const noexcept { return fd_; }
    const std::string& getName() const noexcept { return name_; }

private:
    int fd_{-1};
    std::string name_;
};

#endif // TAP_DEVICE_HPP 