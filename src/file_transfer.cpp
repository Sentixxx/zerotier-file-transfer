#include "file_transfer.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

FileTransfer::FileTransfer() {
    port_ = -1;
}

bool FileTransfer::setIp(const std::string& ip_addr) {
    if (ip_addr.empty()) {
        return false;
    }
    ip_addr_ = ip_addr;
    return true;
}

bool FileTransfer::setPort(int port) {
    if (port <= 0) {
        return false;
    }
    port_ = port;
    return true;
}

bool FileTransfer::setFilePath(const std::string& file_path) {
    if (file_path.empty()) {
        return false;
    }
    file_path_ = file_path;
    return true;
}

bool FileTransfer::sendFile(const std::string& msg, const std::string& file_path) {
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == kInvalidSocket) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }

    // 设置服务器地址
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);
    server_addr_.sin_addr.s_addr = inet_addr(ip_addr_.c_str());

    // 连接服务器
    if (connect(server_socket_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        close(server_socket_);
        return false;
    }

    // 发送消息
    if (!msg.empty()) {
        send(server_socket_, msg.c_str(), msg.length(), 0);
    }

    // 打开并发送文件
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        close(server_socket_);
        return false;
    }

    char buffer[kBufferSize];
    while (!file.eof()) {
        file.read(buffer, kBufferSize);
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            send(server_socket_, buffer, bytes_read, 0);
        }
    }

    file.close();
    close(server_socket_);
    return true;
}

bool FileTransfer::recvFile(const std::string& msg, const std::string& file_path) {
    // 创建socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == kInvalidSocket) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }

    // 设置服务器地址
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);
    server_addr_.sin_addr.s_addr = inet_addr(ip_addr_.c_str());

    // 绑定socket
    if (bind(server_socket_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(server_socket_);
        return false;
    }

    // 监听连接
    if (listen(server_socket_, 1) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_socket_);
        return false;
    }

    // 接受连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        std::cerr << "Accept fai`led" << std::endl;
        close(server_socket_);
        return false;
    }

    // 如果有消息，接收消息
    if (!msg.empty()) {
        char msg_buffer[kBufferSize];
        recv(client_socket, msg_buffer, msg.length(), 0);
    }

    // 打开文件准备写入
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        close(client_socket);
        close(server_socket_);
        return false;
    }

    // 接收文件内容
    char buffer[kBufferSize];
    ssize_t bytes_received;
    while ((bytes_received = recv(client_socket, buffer, kBufferSize, 0)) > 0) {
        file.write(buffer, bytes_received);
    }

    file.close();
    close(client_socket);
    close(server_socket_);
    return true;
}
