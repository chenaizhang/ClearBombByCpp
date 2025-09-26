#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace clearbomb {
namespace {

std::tm local_time_from_time_t(std::time_t time)
{
#ifdef _WIN32
    std::tm result{};
    localtime_s(&result, &time);
    return result;
#else
    std::tm result{};
    localtime_r(&time, &result);
    return result;
#endif
}

std::string format_date(const std::tm& tm)
{
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

}  // namespace

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : level_(LogLevel::Info)
    , console_enabled_(true)
    , rotation_index_(0)
{}

void Logger::set_level(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::enable_console_logging(bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    console_enabled_ = enabled;
}

void Logger::set_log_directory(const std::string& directory, std::string base_filename, std::size_t max_file_size_bytes)
{
    std::lock_guard<std::mutex> lock(mutex_);

    file_target_ = FileTarget{
        .directory = directory,
        .base_filename = std::move(base_filename),
        .max_file_size_bytes = max_file_size_bytes
    };

    rotation_index_ = 0;
    current_date_.clear();
    current_file_path_.clear();

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        std::cerr << "[Logger] Failed to create log directory '" << directory << "': " << ec.message() << std::endl;
    }
}

void Logger::log(
    LogLevel level,
    std::string_view module,
    std::string_view function,
    int line,
    const std::string& message
)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < level_) {
        return;
    }

    const std::string timestamp = current_timestamp_string();
    const std::string level_text = level_to_string(level);
    const std::string thread_id = thread_id_string();

    std::ostringstream formatted;
    formatted << '[' << timestamp << "] [" << level_text << "] [" << thread_id << "] [" << module
              << '.' << function << ':' << line << "] - " << message;
    const std::string line_text = formatted.str();

    if (console_enabled_) {
        std::cout << line_text << std::endl;
    }

    if (file_target_) {
        const std::size_t payload_size = line_text.size() + 1;  // include newline
        ensure_file_target_ready(payload_size);
        if (file_stream_.is_open()) {
            file_stream_ << line_text << '\n';
            file_stream_.flush();
        }
    }
}

void Logger::ensure_file_target_ready(std::size_t message_payload_size)
{
    if (!file_target_) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = local_time_from_time_t(now_time_t);
    const std::string today = format_date(local_tm);

    if (today != current_date_) {
        current_date_ = today;
        rotation_index_ = 0;
        current_file_path_.clear();
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }

    if (!file_stream_.is_open()) {
        open_stream_for_rotation();
    }

    while (file_stream_.is_open()) {
        std::uintmax_t current_size = 0;
        std::error_code ec;
        if (!current_file_path_.empty()) {
            current_size = std::filesystem::file_size(current_file_path_, ec);
            if (ec) {
                current_size = 0;
            }
        }

        if (file_target_->max_file_size_bytes > 0 && current_size + message_payload_size > file_target_->max_file_size_bytes) {
            file_stream_.close();
            ++rotation_index_;
            open_stream_for_rotation();
            continue;
        }

        break;
    }
}

void Logger::open_stream_for_rotation()
{
    if (!file_target_) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = local_time_from_time_t(now_time_t);
    const std::string today = format_date(local_tm);

    if (current_date_.empty()) {
        current_date_ = today;
    }

    const std::filesystem::path directory = file_target_->directory;
    const std::string base = file_target_->base_filename + '_' + current_date_;

    std::filesystem::path candidate = directory / base;
    if (rotation_index_ > 0) {
        candidate = directory / (base + '_' + std::to_string(rotation_index_));
    }
    candidate += ".log";

    current_file_path_ = candidate.string();
    file_stream_.open(candidate, std::ios::app);
    if (!file_stream_.is_open()) {
        std::cerr << "[Logger] Failed to open log file '" << candidate.string() << "'" << std::endl;
    }
}

std::string Logger::level_to_string(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Critical:
        return "CRITICAL";
    }
    return "INFO";
}

std::string Logger::current_timestamp_string()
{
    const auto now = std::chrono::system_clock::now();
    const auto now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = local_time_from_time_t(now_time_t);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::thread_id_string()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

}  // namespace clearbomb
