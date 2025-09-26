#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "GameEngine.hpp"

namespace clearbomb {

class ApiServer {
public:
    explicit ApiServer(std::shared_ptr<GameEngine> engine, unsigned short port = 8080);
    ~ApiServer();

    void start();
    void stop();

private:
    std::shared_ptr<GameEngine> engine_;
    unsigned short port_;
    std::atomic<bool> running_ {false};
    std::thread server_thread_;
    int server_fd_ {-1};
    mutable std::mutex engine_mutex_;

    void run_event_loop();
    void handle_client(int client_fd);
    static std::string build_http_response(int status_code, const std::string& body);
    static std::string build_error_response(int status_code, const std::string& message);
    static std::string status_to_string(GameStatus status);

    std::string handle_get_board() const;
    std::string handle_post_reveal(const std::string& body);
    std::string handle_post_flag(const std::string& body);
    std::string handle_post_auto_mark(const std::string& body);
    std::string handle_post_reset(const std::string& body);

    static std::optional<Position> parse_position(const std::string& body);
    static std::optional<SelectionRect> parse_selection(const std::string& body);
    static std::optional<BoardConfig> parse_board_config(const std::string& body);
    std::string serialize_board_snapshot(const BoardSnapshot& snapshot) const;
    std::string serialize_cells(const std::vector<Cell>& cells) const;
    std::string serialize_cell(const Cell& cell) const;
};

}  // namespace clearbomb
