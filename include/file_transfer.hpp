#ifndef FILE_TRANSFER_HPP
#define FILE_TRANSFER_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string_view>

class FileTransfer {
public:
    FileTransfer() = default;
    ~FileTransfer() {
        if (sock_fd_ >= 0) close(sock_fd_);
    }

    bool sendFile(const std::string& msg, const std::string& file_path);
    bool recvFile(const std::string& msg, const std::string& file_path);
    
    void setAddress(const std::string& ip, int port) {
        ip_addr_ = ip;
        port_ = port;
    }

private:
    static constexpr int kBufferSize = 8192;
    int sock_fd_{-1};
    std::string ip_addr_;
    int port_{0};
    
    bool initTcpConnection(const std::string& ip, int port, bool is_server);
};

#endif // FILE_TRANSFER_HPP 