#pragma once

#include <cstddef>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace clearbomb {

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    [[nodiscard]] LogLevel level() const noexcept;

    void enable_console_logging(bool enabled);

    void set_log_directory(
        const std::string& directory,
        std::string base_filename = "minesweeper",
        std::size_t max_file_size_bytes = 5 * 1024 * 1024
    );

    void log(
        LogLevel level,
        std::string_view module,
        std::string_view function,
        int line,
        const std::string& message
    );

private:
    Logger();

    struct FileTarget {
        std::string directory;
        std::string base_filename;
        std::size_t max_file_size_bytes;
    };

    void ensure_file_target_ready(std::size_t message_payload_size);
    void open_stream_for_rotation();
    static std::string level_to_string(LogLevel level);
    static std::string current_timestamp_string();
    static std::string thread_id_string();

    mutable std::mutex mutex_;
    LogLevel level_;
    bool console_enabled_;
    std::optional<FileTarget> file_target_;
    std::string current_date_;
    int rotation_index_;
    std::string current_file_path_;
    std::ofstream file_stream_;
};

}  // namespace clearbomb

#include <sstream>

#define CLEARBOMB_LOG_INTERNAL(level, module, message_expr)                           \
    do {                                                                             \
        std::ostringstream _clearbomb_log_stream;                                    \
        _clearbomb_log_stream << message_expr;                                       \
        ::clearbomb::Logger::instance().log(                                         \
            level,                                                                   \
            module,                                                                  \
            __func__,                                                                \
            __LINE__,                                                                \
            _clearbomb_log_stream.str()                                              \
        );                                                                           \
    } while (false)

#define LOG_DEBUG(module, message) CLEARBOMB_LOG_INTERNAL(::clearbomb::LogLevel::Debug, module, message)
#define LOG_INFO(module, message) CLEARBOMB_LOG_INTERNAL(::clearbomb::LogLevel::Info, module, message)
#define LOG_WARNING(module, message) CLEARBOMB_LOG_INTERNAL(::clearbomb::LogLevel::Warning, module, message)
#define LOG_ERROR(module, message) CLEARBOMB_LOG_INTERNAL(::clearbomb::LogLevel::Error, module, message)
#define LOG_CRITICAL(module, message) CLEARBOMB_LOG_INTERNAL(::clearbomb::LogLevel::Critical, module, message)

