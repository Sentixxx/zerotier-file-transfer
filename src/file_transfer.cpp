#include "file_transfer.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

bool FileTransfer::sendFile(const std::string& msg, const std::string& file_path) {
    if (ip_addr_.empty() || port_ <= 0) {
        std::cerr << "Address not set" << std::endl;
        return false;
    }

    // 检查文件是否存在并获取大小
    fs::path fs_path(file_path);
    if (!fs::exists(fs_path)) {
        std::cerr << "File does not exist: " << file_path << std::endl;
        return false;
    }
    uint64_t file_size = fs::file_size(fs_path);

    // 作为客户端连接服务器
    if (!initTcpConnection(ip_addr_, port_, false)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return false;
    }

    // 发送文件大小
    if (send(sock_fd_, &file_size, sizeof(file_size), 0) != sizeof(file_size)) {
        std::cerr << "Failed to send file size" << std::endl;
        close(sock_fd_);
        return false;
    }

    // 发送消息
    if (!msg.empty()) {
        uint32_t msg_len = msg.length();
        if (send(sock_fd_, &msg_len, sizeof(msg_len), 0) != sizeof(msg_len) ||
            send(sock_fd_, msg.c_str(), msg_len, 0) != static_cast<ssize_t>(msg_len)) {
            std::cerr << "Failed to send message" << std::endl;
            close(sock_fd_);
            return false;
        }
    }

    // 打开并发送文件
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        close(sock_fd_);
        return false;
    }

    char buffer[kBufferSize];
    uint64_t total_sent = 0;
    while (total_sent < file_size && !file.eof()) {
        file.read(buffer, kBufferSize);
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            ssize_t sent = send(sock_fd_, buffer, bytes_read, 0);
            if (sent < 0) {
                std::cerr << "Send error: " << strerror(errno) << std::endl;
                close(sock_fd_);
                file.close();
                return false;
            }
            total_sent += sent;
        }
    }

    file.close();
    return true;
}

bool FileTransfer::recvFile(const std::string& msg, const std::string& file_path) {
    if (ip_addr_.empty() || port_ <= 0) {
        std::cerr << "Address not set" << std::endl;
        return false;
    }

    // 作为服务器等待连接
    if (!initTcpConnection(ip_addr_, port_, true)) {
        std::cerr << "Failed to initialize server" << std::endl;
        return false;
    }

    // 接受客户端连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(sock_fd_, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        close(sock_fd_);
        return false;
    }

    // 接收文件大小
    uint64_t file_size;
    if (recv(client_fd, &file_size, sizeof(file_size), MSG_WAITALL) != sizeof(file_size)) {
        std::cerr << "Failed to receive file size" << std::endl;
        close(client_fd);
        close(sock_fd_);
        return false;
    }

    // 如果有消息，接收消息
    if (!msg.empty()) {
        uint32_t msg_len;
        char msg_buffer[kBufferSize];
        if (recv(client_fd, &msg_len, sizeof(msg_len), MSG_WAITALL) != sizeof(msg_len) ||
            recv(client_fd, msg_buffer, msg_len, MSG_WAITALL) != static_cast<ssize_t>(msg_len)) {
            std::cerr << "Failed to receive message" << std::endl;
            close(client_fd);
            close(sock_fd_);
            return false;
        }
    }

    // 打开文件准备写入
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        close(client_fd);
        close(sock_fd_);
        return false;
    }

    // 接收文件内容
    char buffer[kBufferSize];
    uint64_t total_received = 0;
    while (total_received < file_size) {
        ssize_t bytes_received = recv(client_fd, buffer, 
            std::min(sizeof(buffer), static_cast<size_t>(file_size - total_received)), 0);
        if (bytes_received <= 0) {
            std::cerr << "Receive error: " << strerror(errno) << std::endl;
            file.close();
            close(client_fd);
            close(sock_fd_);
            return false;
        }
        file.write(buffer, bytes_received);
        total_received += bytes_received;
    }

    file.close();
    close(client_fd);
    return true;
}

bool FileTransfer::initTcpConnection(const std::string& ip, int port, bool is_server) {
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_ < 0) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (is_server) {
        // 服务器模式：绑定并监听
        if (bind(sock_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(sock_fd_, 1) < 0) return false;
    } else {
        // 客户端模式：连接服务器
        if (connect(sock_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
    }
    return true;
}
