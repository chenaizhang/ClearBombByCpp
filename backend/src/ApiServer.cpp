#include "ApiServer.hpp"

#include <iostream>

namespace clearbomb {

ApiServer::ApiServer(std::shared_ptr<GameEngine> engine)
    : engine_(std::move(engine))
{
    // TODO: Validate engine dependency once injection strategy is finalized.
}

void ApiServer::start()
{
    if (running_) {
        return;
    }
    running_ = true;
    std::cout << "API server starting..." << std::endl;
    run_event_loop();
}

void ApiServer::stop()
{
    running_ = false;
    std::cout << "API server stopping." << std::endl;
    // TODO: Shut down network listeners gracefully.
}

void ApiServer::run_event_loop()
{
    // TODO: Implement async event loop with chosen networking library.
    if (!engine_) {
        std::cerr << "GameEngine not initialized." << std::endl;
        return;
    }
}

}  // namespace clearbomb
