#include "vport.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <unistd.h>

VPort::VPort(const std::string& server_ip, int server_port) {
    try {
        // 使用generateUniqueTapName获取可用的TAP设备名
        std::string tap_name = generateUniqueTapName();
        tap_device_ = TapDevice(tap_name);

        // 创建socket
        vport_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (vport_sockfd_ < 0) {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        // 设置VSwitch地址
        vswitch_addr_.sin_family = AF_INET;
        vswitch_addr_.sin_port = htons(server_port);
        if (inet_pton(AF_INET, server_ip.c_str(), &vswitch_addr_.sin_addr) != 1) {
            throw std::runtime_error("Failed to set IP address: " + std::string(strerror(errno)));
        }

        std::cout << "[VPort] TAP device name: " << tap_device_.getName()
                  << ", VSwitch: " << server_ip << ":" << server_port << std::endl;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to create TAP device: " + std::string(e.what()));
    }
}

VPort::~VPort() {
    stop();
    if (vport_sockfd_ >= 0) close(vport_sockfd_);
}

void VPort::start() {
    running_ = true;
    up_forwarder_ = std::make_unique<std::thread>(&VPort::forwardEtherDataToVSwitch, this);
    down_forwarder_ = std::make_unique<std::thread>(&VPort::forwardEtherDataToTap, this);
}

void VPort::stop() {
    running_ = false;
    if (up_forwarder_ && up_forwarder_->joinable()) {
        up_forwarder_->join();
    }
    if (down_forwarder_ && down_forwarder_->joinable()) {
        down_forwarder_->join();
    }
}

void VPort::forwardEtherDataToVSwitch() {
    char ether_data[ETHER_MAX_LEN];
    while (running_) {
        int ether_datasz = read(tap_device_.getFd(), ether_data, sizeof(ether_data));
        if (ether_datasz > 0) {
            assert(ether_datasz >= 14);
            const auto* hdr = reinterpret_cast<const struct ether_header*>(ether_data);

            ssize_t sendsz = sendto(vport_sockfd_, ether_data, ether_datasz, 0,
                                  reinterpret_cast<struct sockaddr*>(&vswitch_addr_),
                                  sizeof(vswitch_addr_));
            
            if (sendsz != ether_datasz) {
                std::cerr << "sendto size mismatch: ether_datasz=" 
                         << ether_datasz << ", sendsz=" << sendsz << std::endl;
            }

            printf("[VPort] Sent to VSwitch:"
                   " dhost<%02x:%02x:%02x:%02x:%02x:%02x>"
                   " shost<%02x:%02x:%02x:%02x:%02x:%02x>"
                   " type<%04x>"
                   " datasz=<%d>\n",
                   hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2],
                   hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
                   hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2],
                   hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
                   ntohs(hdr->ether_type), ether_datasz);
        }
    }
}

void VPort::forwardEtherDataToTap() {
    char ether_data[ETHER_MAX_LEN];
    while (running_) {
        socklen_t vswitch_addr_len = sizeof(vswitch_addr_);
        int ether_datasz = recvfrom(vport_sockfd_, ether_data, sizeof(ether_data), 0,
                                  reinterpret_cast<struct sockaddr*>(&vswitch_addr_),
                                  &vswitch_addr_len);
        
        if (ether_datasz > 0) {
            assert(ether_datasz >= 14);
            const auto* hdr = reinterpret_cast<const struct ether_header*>(ether_data);

            ssize_t sendsz = write(tap_device_.getFd(), ether_data, ether_datasz);
            if (sendsz != ether_datasz) {
                std::cerr << "write size mismatch: ether_datasz="
                         << ether_datasz << ", sendsz=" << sendsz << std::endl;
            }

            printf("[VPort] Forward to TAP device:"
                   " dhost<%02x:%02x:%02x:%02x:%02x:%02x>"
                   " shost<%02x:%02x:%02x:%02x:%02x:%02x>"
                   " type<%04x>"
                   " datasz=<%d>\n",
                   hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2],
                   hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
                   hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2],
                   hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
                   ntohs(hdr->ether_type), ether_datasz);
        }
    }
}

std::string generateUniqueTapName() {
    for (int i = 0; i < 10000; ++i) {
        std::string name = "tap" + std::to_string(i);
        if (access(("/sys/class/net/" + name).c_str(), F_OK) == -1) {
            return name;
        }
    }
    throw std::runtime_error("无法找到可用的TAP设备名称");
} 