#pragma once

#include <memory>

#include "GameEngine.hpp"

namespace clearbomb {

class ApiServer {
public:
    explicit ApiServer(std::shared_ptr<GameEngine> engine);

    void start();
    void stop();

    // TODO: Expose HTTP/WebSocket handlers for reveal/flag/selection endpoints.

private:
    std::shared_ptr<GameEngine> engine_;
    bool running_ {false};

    // TODO: Integrate with chosen networking stack (Boost.Beast, Pistache, Drogon, etc.).
    void run_event_loop();
};

}  // namespace clearbomb
