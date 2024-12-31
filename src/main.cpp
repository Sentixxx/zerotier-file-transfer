#include "vport.hpp"
#include "file_transfer.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <cstring>
#include <thread>
#include <chrono>

namespace {
    std::unique_ptr<VPort> g_vport;
    volatile sig_atomic_t g_running = 1;
    volatile sig_atomic_t g_transfer_running = 1;

    void signalHandler(int) {
        g_running = 0;
        g_transfer_running = 0;
        std::cout << "\n检测到中断信号，正在退出...\n" << std::flush;
    }

    void clearInputBuffer() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    bool getTransferMode(int& mode) {
        while (true) {
            std::cout << "\n请选择传输模式:\n"
                     << "1. 发送文件\n"
                     << "2. 接收文件\n" 
                     << "0. 退出\n"
                     << "您的选择: ";
            
            if (!(std::cin >> mode)) {
                clearInputBuffer();
                std::cout << "输入无效,请输入数字.\n";
                continue;
            }

            if (mode < 0 || mode > 2) {
                std::cout << "选择无效,请选择 0-2.\n";
                continue;
            }

            return mode != 0;
        }
    }

    std::string getFilePath() {
        std::string file_path;
        std::cout << "请输入文件路径: ";
        clearInputBuffer();
        std::getline(std::cin, file_path);
        return file_path;
    }

    bool validateFilePath(const std::string& path, bool checkExists) {
        try {
            if (path.empty()) {
                std::cout << "文件路径不能为空.\n";
                return false;
            }

            struct stat path_stat;
            
            if (checkExists) {
                if (stat(path.c_str(), &path_stat) != 0) {
                    std::cout << "文件不存在: " << path << "\n";
                    return false;
                }
            } else {
                char* path_copy = strdup(path.c_str());
                char* dir_path = dirname(path_copy);
                
                if (stat(dir_path, &path_stat) != 0) {
                    std::cout << "目标目录不存在: " << dir_path << "\n";
                    free(path_copy);
                    return false;
                }
                free(path_copy);
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cout << "文件路径验证失败: " << e.what() << "\n";
            return false;
        }
    }
}

bool validateAddress(const std::string& addr, int port) {
    try {
        // 检查端口范围
        if (port <= 0 || port > 65535) {
            std::cout << "无效的端口号: " << port << "\n"; 
            return false;
        }

        if(addr == "") {
            return true;
        }
        
        // 检查IP地址格式
        struct sockaddr_in sa;
        if (inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr)) != 1) {
            std::cout << "无效的IP地址格式: " << addr << "\n";
            return false;
        }



        return true;
    }
    catch (const std::exception& e) {
        std::cout << "地址验证失败: " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char const *argv[]) {
    try {
        if (argc != 3) {
            throw std::runtime_error("用法: vport {服务器IP} {服务器端口}");
        }

        const std::string server_ip = argv[1];
        const int server_port = std::stoi(argv[2]);

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        std::cout << "正在连接到服务器 " << server_ip << ":" << server_port << "...\n";
        
        g_vport = std::make_unique<VPort>(server_ip, server_port);
        g_vport->start();
        
        std::cout << "连接成功!\n";

        while (g_running) {
            int mode;
            if (!getTransferMode(mode)) {
                break;
            }
            std::string file_path = getFilePath();
            
            // 发送模式需要检查源文件存在,接收模式检查目标路径有效
            if (!validateFilePath(file_path, mode == 1)) {
                continue;
            }

            FileTransfer ft;
            try {
                switch (mode) {
                    case 1: {
                        std::cout << "请输入远端地址: ";
                        std::string remote_addr;
                        std::getline(std::cin, remote_addr);
                        std::cout << "请输入远端端口号：";
                        int remote_port;
                        std::cin >> remote_port;
                        clearInputBuffer();
                        if(!validateAddress(remote_addr,remote_port)) {
                            continue;
                        }
                        ft.setAddress(remote_addr,remote_port);
                        
                        std::cout << "请确保接收方已经在等待接收...\n";
                        std::cout << "按回车键开始发送...";
                        std::cin.get();
                        
                        std::cout << "正在发送文件 " << file_path << "...\n";
                        if (ft.sendFile("", file_path)) {
                            std::cout << "文件发送成功.\n";
                        } else {
                            std::cout << "文件发送失败.\n";
                        }
                        break;
                    }
                    case 2: {
                        std::cout << "请输入监听端口号: ";
                        int listen_port;
                        std::cin >> listen_port;
                        clearInputBuffer();
                        if(!validateAddress("", listen_port)) {
                            continue;
                        }
                        ft.setAddress("", listen_port);
                        
                        std::cout << "正在等待发送方连接 (端口: " << listen_port << ")...\n";
                        std::cout << "按Ctrl+C取消等待\n";
                        
                        g_transfer_running = 1;
                        bool result = false;
                        
                        while (g_transfer_running) {
                            try {
                                result = ft.recvFile("", file_path);
                                if (result) break;
                                
                                if (!g_transfer_running) {
                                    std::cout << "\n传输已取消\n";
                                    break;
                                }
                                
                                std::cout << "接收失败,正在重试...\n";
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                            }
                            catch (const std::exception& e) {
                                if (!g_transfer_running) {
                                    std::cout << "\n传输已取消\n";
                                    break;
                                }
                                std::cerr << "错误: " << e.what() << std::endl;
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                            }
                        }
                        
                        if (result) {
                            std::cout << "文件接收成功.\n";
                        } else if (g_transfer_running) {
                            std::cout << "文件接收失败.\n";
                        }
                        break;
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "操作失败: " << e.what() << std::endl;
            }
        }

        std::cout << "\n正在关闭连接...\n";
        g_vport->stop();
        g_vport.reset();
        std::cout << "已安全退出.\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}