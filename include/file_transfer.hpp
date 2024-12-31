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
    
    FileTransfer(int port, const std::string& ip_addr, const std::string& file_path)
        : port_(port), ip_addr_(ip_addr), file_path_(file_path) {}
    
    virtual ~FileTransfer() = default;
    
    FileTransfer(const FileTransfer&) = delete;
    FileTransfer& operator=(const FileTransfer&) = delete;
    
    FileTransfer(FileTransfer&&) noexcept = default;
    FileTransfer& operator=(FileTransfer&&) noexcept = default;
    
    bool setIp(const std::string& ip_addr);
    bool setPort(int port);
    bool setFilePath(const std::string& file_path);
    bool sendFile(const std::string& msg, const std::string& file_path);
    bool recvFile(const std::string& msg, const std::string& file_path);
    
    [[nodiscard]] const std::string& getIpAddr() const noexcept { return ip_addr_; }
    [[nodiscard]] int getPort() const noexcept { return port_; }
    [[nodiscard]] const std::string& getFilePath() const noexcept { return file_path_; }

private:
    static constexpr int kBufferSize = 1024;
    static constexpr int kInvalidSocket = -1;

    int port_{0};
    std::string ip_addr_;
    std::string file_path_;

    int server_socket_{kInvalidSocket};
    struct sockaddr_in server_addr_{};
};

#endif // FILE_TRANSFER_HPP 