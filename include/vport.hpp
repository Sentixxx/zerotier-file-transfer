#ifndef VPORT_HPP
#define VPORT_HPP

#include <string>
#include <memory>
#include <thread>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include "tap_device.hpp"

class VPort {
public:
    VPort(const std::string& server_ip, int server_port);
    ~VPort();

    void start();
    void stop();

    static void setVerbose(bool verbose) {
        verbose_ = verbose;
    }

private:
    static constexpr int kBufferSize = 1024;
    
    void forwardEtherDataToVSwitch();
    void forwardEtherDataToTap();
    
    TapDevice tap_device_;
    int vport_sockfd_{-1};
    struct sockaddr_in vswitch_addr_{};
    bool running_{false};
    
    std::unique_ptr<std::thread> up_forwarder_;
    std::unique_ptr<std::thread> down_forwarder_;

    static bool verbose_;
};

#endif // VPORT_HPP 