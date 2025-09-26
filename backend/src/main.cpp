#include "ApiServer.hpp"
#include "GameEngine.hpp"
#include "Logger.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

int main(int argc, char* argv[])
{
    using namespace clearbomb;

    auto& logger = Logger::instance();
    logger.set_level(LogLevel::Info);

    try {
        const auto log_directory = std::filesystem::current_path() / "logs";
        logger.set_log_directory(log_directory.string());
        LOG_INFO("Application", "Logging configured at " << log_directory);
    } catch (const std::exception& error) {
        LOG_WARNING("Application", "Unable to configure file logging: " << error.what());
    }

    unsigned short port = 8080;
    if (argc > 1) {
        try {
            const int parsed = std::stoi(argv[1]);
            if (parsed > 0 && parsed <= 65535) {
                port = static_cast<unsigned short>(parsed);
            }
        } catch (const std::exception&) {
            std::cerr << "Invalid port argument. Falling back to 8080." << std::endl;
            LOG_WARNING("Application", "Invalid CLI port argument - falling back to 8080");
        }
    }

    auto engine = std::make_shared<GameEngine>();
    ApiServer server{engine, port};

    server.start();
    LOG_INFO("Application", "Clear Bomb server running on port " << port);
    std::cout << "Clear Bomb server running on port " << port << ". Press Ctrl+C to exit." << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
