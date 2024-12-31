#include "tap_device.hpp"
#include <stdexcept>
#include <cstring>

TapDevice::TapDevice(const std::string& dev_name) : name_(dev_name) {
    // 打开 TUN/TAP 设备
    fd_ = open("/dev/net/tun", O_RDWR);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open /dev/net/tun: " + 
                               std::string(strerror(errno)));
    }

    // 配置 TAP 设备
    struct ifreq ifr{};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    
    if (!name_.empty()) {
        strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ - 1);
    }

    if (ioctl(fd_, TUNSETIFF, &ifr) < 0) {
        close(fd_);
        throw std::runtime_error("Failed to ioctl TUNSETIFF: " + 
                               std::string(strerror(errno)));
    }

    name_ = ifr.ifr_name;
}

TapDevice::~TapDevice() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

TapDevice::TapDevice(TapDevice&& other) noexcept 
    : fd_(other.fd_), name_(std::move(other.name_)) {
    other.fd_ = -1;
}

TapDevice& TapDevice::operator=(TapDevice&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = other.fd_;
        name_ = std::move(other.name_);
        other.fd_ = -1;
    }
    return *this;
} 