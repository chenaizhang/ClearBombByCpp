#include "ApiServer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace clearbomb {

namespace {
std::string reason_phrase(int status_code)
{
    switch (status_code) {
    case 200:
        return "OK";
    case 204:
        return "No Content";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 500:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

std::string cell_state_to_string(CellState state)
{
    switch (state) {
    case CellState::Hidden:
        return "hidden";
    case CellState::Revealed:
        return "revealed";
    case CellState::Flagged:
        return "flagged";
    }
    return "hidden";
}

bool is_whitespace_only(const std::string& text)
{
    return std::all_of(text.begin(), text.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
}

std::string format_bool(bool value)
{
    return value ? "true" : "false";
}

}  // namespace

ApiServer::ApiServer(std::shared_ptr<GameEngine> engine, unsigned short port)
    : engine_(std::move(engine))
    , port_(port)
{
    if (!engine_) {
        throw std::invalid_argument("ApiServer requires a valid GameEngine instance.");
    }
}

ApiServer::~ApiServer()
{
    stop();
}

void ApiServer::start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return;
    }

    server_thread_ = std::thread(&ApiServer::run_event_loop, this);
}

void ApiServer::stop()
{
    if (!running_.exchange(false)) {
        return;
    }

    if (server_fd_ >= 0) {
        ::shutdown(server_fd_, SHUT_RDWR);
        ::close(server_fd_);
        server_fd_ = -1;
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void ApiServer::run_event_loop()
{
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::perror("socket");
        running_ = false;
        return;
    }

    int enable = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        std::perror("setsockopt");
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::perror("bind");
        ::close(server_fd_);
        server_fd_ = -1;
        running_ = false;
        return;
    }

    if (listen(server_fd_, 16) < 0) {
        std::perror("listen");
        ::close(server_fd_);
        server_fd_ = -1;
        running_ = false;
        return;
    }

    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);

        timeval timeout {1, 0};
        const int activity = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("select");
            break;
        }

        if (!running_) {
            break;
        }

        if (FD_ISSET(server_fd_, &read_fds)) {
            sockaddr_in client_addr {};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            if (client_fd < 0) {
                std::perror("accept");
                continue;
            }

            std::thread(&ApiServer::handle_client, this, client_fd).detach();
        }
    }

    if (server_fd_ >= 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }
}

void ApiServer::handle_client(int client_fd)
{
    std::string request;
    char buffer[4096];
    ssize_t bytes_read = 0;

    while ((bytes_read = ::recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, buffer + bytes_read);
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    if (bytes_read < 0) {
        std::perror("recv");
        ::close(client_fd);
        return;
    }

    const auto header_end = request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        const auto response = build_error_response(400, "Invalid HTTP request");
        ::send(client_fd, response.c_str(), response.size(), 0);
        ::close(client_fd);
        return;
    }

    std::string headers = request.substr(0, header_end);
    std::string body;

    std::regex content_length_regex("Content-Length:\\s*(\\d+)", std::regex::icase);
    std::smatch length_match;
    std::size_t content_length = 0;
    if (std::regex_search(headers, length_match, content_length_regex)) {
        content_length = static_cast<std::size_t>(std::stoul(length_match[1]));
    }

    const std::size_t body_start = header_end + 4;
    if (body_start < request.size()) {
        body = request.substr(body_start);
    }

    while (body.size() < content_length) {
        bytes_read = ::recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            break;
        }
        body.append(buffer, buffer + bytes_read);
    }

    std::istringstream header_stream(headers);
    std::string request_line;
    std::getline(header_stream, request_line);
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    std::string method;
    std::string path;
    std::string version;
    request_line_stream >> method >> path >> version;

    std::string response;

    if (method == "OPTIONS") {
        response = build_http_response(204, "");
    } else if (method == "GET" && path == "/api/board") {
        response = handle_get_board();
    } else if (method == "POST" && path == "/api/reveal") {
        response = handle_post_reveal(body);
    } else if (method == "POST" && path == "/api/flag") {
        response = handle_post_flag(body);
    } else if (method == "POST" && path == "/api/auto-mark") {
        response = handle_post_auto_mark(body);
    } else if (method == "POST" && path == "/api/reset") {
        response = handle_post_reset(body);
    } else {
        response = build_error_response(404, "Endpoint not found");
    }

    ::send(client_fd, response.c_str(), response.size(), 0);
    ::close(client_fd);
}

std::string ApiServer::build_http_response(int status_code, const std::string& body)
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << ' ' << reason_phrase(status_code) << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << body;
    return response.str();
}

std::string ApiServer::build_error_response(int status_code, const std::string& message)
{
    std::ostringstream payload;
    payload << "{\"error\":\"" << message << "\"}";
    return build_http_response(status_code, payload.str());
}

std::string ApiServer::status_to_string(GameStatus status)
{
    switch (status) {
    case GameStatus::Playing:
        return "playing";
    case GameStatus::Victory:
        return "victory";
    case GameStatus::Defeat:
        return "defeat";
    }
    return "playing";
}

std::string ApiServer::handle_get_board() const
{
    std::lock_guard<std::mutex> guard(engine_mutex_);
    const auto snapshot = engine_->snapshot();
    return build_http_response(200, serialize_board_snapshot(snapshot));
}

std::string ApiServer::handle_post_reveal(const std::string& body)
{
    const auto position = parse_position(body);
    if (!position) {
        return build_error_response(400, "Invalid reveal payload");
    }

    std::lock_guard<std::mutex> guard(engine_mutex_);
    const auto result = engine_->reveal_cell(*position);
    const auto snapshot = engine_->snapshot();

    std::ostringstream payload;
    payload << "{\"updatedCells\":" << serialize_cells(result.updated_cells)
            << ",\"hitMine\":" << format_bool(result.hit_mine)
            << ",\"victory\":" << format_bool(result.victory)
            << ",\"flagsRemaining\":" << result.flags_remaining
            << ",\"status\":\"" << status_to_string(snapshot.status) << "\"}";

    return build_http_response(200, payload.str());
}

std::string ApiServer::handle_post_flag(const std::string& body)
{
    const auto position = parse_position(body);
    if (!position) {
        return build_error_response(400, "Invalid flag payload");
    }

    std::lock_guard<std::mutex> guard(engine_mutex_);
    const auto result = engine_->toggle_flag(*position);
    const auto snapshot = engine_->snapshot();

    std::ostringstream payload;
    payload << "{\"updatedCell\":" << serialize_cell(result.updated_cell)
            << ",\"flagsRemaining\":" << result.flags_remaining
            << ",\"victory\":" << format_bool(result.victory)
            << ",\"status\":\"" << status_to_string(snapshot.status) << "\"}";

    return build_http_response(200, payload.str());
}

std::string ApiServer::handle_post_auto_mark(const std::string& body)
{
    const auto selection = parse_selection(body);
    if (!selection) {
        return build_error_response(400, "Invalid selection payload");
    }

    std::lock_guard<std::mutex> guard(engine_mutex_);
    const auto auto_result = engine_->auto_mark(*selection);
    const auto snapshot = engine_->snapshot();

    std::ostringstream payload;
    if (auto_result) {
        payload << "{\"flaggedCells\":" << serialize_cells(auto_result->flagged_cells)
                << ",\"flagsRemaining\":" << auto_result->flags_remaining
                << ",\"victory\":" << format_bool(auto_result->victory)
                << ",\"status\":\"" << status_to_string(snapshot.status) << "\"}";
    } else {
        payload << "{\"flaggedCells\":[]"
                << ",\"flagsRemaining\":" << snapshot.flags_remaining
                << ",\"victory\":" << format_bool(snapshot.status == GameStatus::Victory)
                << ",\"status\":\"" << status_to_string(snapshot.status) << "\"}";
    }

    return build_http_response(200, payload.str());
}

std::string ApiServer::handle_post_reset(const std::string& body)
{
    std::optional<BoardConfig> config;

    if (!body.empty() && !is_whitespace_only(body)) {
        config = parse_board_config(body);
        if (!config) {
            return build_error_response(400, "Invalid board configuration");
        }
    }

    std::lock_guard<std::mutex> guard(engine_mutex_);
    engine_->reset(config);
    const auto snapshot = engine_->snapshot();
    return build_http_response(200, serialize_board_snapshot(snapshot));
}

std::optional<Position> ApiServer::parse_position(const std::string& body)
{
    std::regex row_regex("\"row\"\\s*:\\s*(\\d+)");
    std::regex column_regex("\"column\"\\s*:\\s*(\\d+)");
    std::smatch row_match;
    std::smatch column_match;

    if (!std::regex_search(body, row_match, row_regex) || !std::regex_search(body, column_match, column_regex)) {
        return std::nullopt;
    }

    Position position{
        static_cast<std::size_t>(std::stoull(row_match[1])),
        static_cast<std::size_t>(std::stoull(column_match[1]))
    };
    return position;
}

std::optional<SelectionRect> ApiServer::parse_selection(const std::string& body)
{
    std::regex row_begin_regex("\"rowBegin\"\\s*:\\s*(\\d+)");
    std::regex col_begin_regex("\"colBegin\"\\s*:\\s*(\\d+)");
    std::regex row_end_regex("\"rowEnd\"\\s*:\\s*(\\d+)");
    std::regex col_end_regex("\"colEnd\"\\s*:\\s*(\\d+)");

    std::smatch match;
    SelectionRect rect{};

    if (!std::regex_search(body, match, row_begin_regex)) {
        return std::nullopt;
    }
    rect.row_begin = static_cast<std::size_t>(std::stoull(match[1]));

    if (!std::regex_search(body, match, row_end_regex)) {
        return std::nullopt;
    }
    rect.row_end = static_cast<std::size_t>(std::stoull(match[1]));

    if (!std::regex_search(body, match, col_begin_regex)) {
        return std::nullopt;
    }
    rect.col_begin = static_cast<std::size_t>(std::stoull(match[1]));

    if (!std::regex_search(body, match, col_end_regex)) {
        return std::nullopt;
    }
    rect.col_end = static_cast<std::size_t>(std::stoull(match[1]));

    return rect;
}

std::optional<BoardConfig> ApiServer::parse_board_config(const std::string& body)
{
    std::regex rows_regex("\"rows\"\\s*:\\s*(\\d+)");
    std::regex columns_regex("\"columns\"\\s*:\\s*(\\d+)");
    std::regex mines_regex("\"mines\"\\s*:\\s*(\\d+)");

    std::smatch rows_match;
    std::smatch columns_match;
    std::smatch mines_match;

    if (!std::regex_search(body, rows_match, rows_regex) ||
        !std::regex_search(body, columns_match, columns_regex) ||
        !std::regex_search(body, mines_match, mines_regex)) {
        return std::nullopt;
    }

    BoardConfig config{
        static_cast<std::size_t>(std::stoull(rows_match[1])),
        static_cast<std::size_t>(std::stoull(columns_match[1])),
        static_cast<std::size_t>(std::stoull(mines_match[1]))
    };
    return config;
}

std::string ApiServer::serialize_board_snapshot(const BoardSnapshot& snapshot) const
{
    std::ostringstream payload;
    payload << "{\"rows\":" << snapshot.rows
            << ",\"columns\":" << snapshot.columns
            << ",\"mines\":" << snapshot.mines
            << ",\"flagsRemaining\":" << snapshot.flags_remaining
            << ",\"status\":\"" << status_to_string(snapshot.status) << "\""
            << ",\"cells\":" << serialize_cells(snapshot.cells) << "}";
    return payload.str();
}

std::string ApiServer::serialize_cells(const std::vector<Cell>& cells) const
{
    std::ostringstream buffer;
    buffer << '[';
    for (std::size_t i = 0; i < cells.size(); ++i) {
        buffer << serialize_cell(cells[i]);
        if (i + 1 < cells.size()) {
            buffer << ',';
        }
    }
    buffer << ']';
    return buffer.str();
}

std::string ApiServer::serialize_cell(const Cell& cell) const
{
    const bool mine_visible = cell.state == CellState::Revealed && cell.is_mine;
    const int adjacent_value = (cell.state == CellState::Revealed && !cell.is_mine) ? cell.adjacent_mines : 0;

    std::ostringstream out;
    out << "{\"row\":" << cell.position.row
        << ",\"column\":" << cell.position.column
        << ",\"state\":\"" << cell_state_to_string(cell.state) << "\""
        << ",\"adjacentMines\":" << adjacent_value
        << ",\"isMine\":" << format_bool(mine_visible)
        << ",\"exploded\":" << format_bool(cell.exploded)
        << '}';
    return out.str();
}

}  // namespace clearbomb
