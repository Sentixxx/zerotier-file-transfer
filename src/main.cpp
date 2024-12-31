#include "vport.hpp"
#include <iostream>
#include <string>
#include <csignal>

namespace {
    std::unique_ptr<VPort> g_vport;
    volatile sig_atomic_t g_running = 1;

    void signalHandler(int) {
        g_running = 0;
    }
}

int main(int argc, char const *argv[]) {
    try {
        // 参数检查
        if (argc != 3) {
            throw std::runtime_error("Usage: vport {server_ip} {server_port}");
        }

        const std::string server_ip = argv[1];
        const int server_port = std::stoi(argv[2]);

        // 设置信号处理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 创建并启动VPort
        g_vport = std::make_unique<VPort>(server_ip, server_port);
        g_vport->start();

        // 等待中断信号
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 优雅退出
        g_vport->stop();
        g_vport.reset();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 