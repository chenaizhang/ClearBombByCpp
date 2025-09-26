#include "ApiServer.hpp"
#include "GameEngine.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

int main(int argc, char* argv[])
{
    using namespace clearbomb;

    unsigned short port = 8080;
    if (argc > 1) {
        try {
            const int parsed = std::stoi(argv[1]);
            if (parsed > 0 && parsed <= 65535) {
                port = static_cast<unsigned short>(parsed);
            }
        } catch (const std::exception&) {
            std::cerr << "Invalid port argument. Falling back to 8080." << std::endl;
        }
    }

    auto engine = std::make_shared<GameEngine>();
    ApiServer server{engine, port};

    server.start();
    std::cout << "Clear Bomb server running on port " << port << ". Press Ctrl+C to exit." << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
