#include "ApiServer.hpp"
#include "GameEngine.hpp"

#include <memory>

int main()
{
    using namespace clearbomb;

    auto engine = std::make_shared<GameEngine>();
    ApiServer server{engine};

    // TODO: Parse CLI arguments or config files to customize server startup.
    server.start();

    return 0;
}
