#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <spdlog/details/file_helper.h>
#include <spdlog/details/os.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "tool_logger.h"

namespace app {

namespace {

class daily_size_file_sink_mt final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit daily_size_file_sink_mt(std::size_t max_size_bytes)
        : max_size_bytes_(max_size_bytes) {
        if (max_size_bytes_ == 0) {
            throw spdlog::spdlog_ex("max_size_bytes must be greater than zero");
        }

        open_latest_file_for_day(to_local_tm(spdlog::log_clock::now()));
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        const auto message_tm = to_local_tm(msg.time);
        if (!is_same_day(message_tm, current_tm_)) {
            open_latest_file_for_day(message_tm);
        }

        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);

        if (formatted.size() >= max_size_bytes_) {
            while (current_size_ != 0) {
                rotate_file();
            }
        } else {
            while (current_size_ + formatted.size() > max_size_bytes_) {
                rotate_file();
            }
        }

        file_helper_.write(formatted);
        current_size_ += formatted.size();
    }

    void flush_() override { file_helper_.flush(); }

private:
    static std::tm to_local_tm(spdlog::log_clock::time_point tp) {
        const auto time = spdlog::log_clock::to_time_t(tp);
        return spdlog::details::os::localtime(time);
    }

    static bool is_same_day(const std::tm &lhs, const std::tm &rhs) {
        return lhs.tm_year == rhs.tm_year && lhs.tm_yday == rhs.tm_yday;
    }

    static spdlog::filename_t make_filename(const std::tm &tm, std::size_t index) {
        std::ostringstream stream;
        stream << "log/log_" << std::put_time(&tm, "%Y_%m_%d") << "._" << std::setfill('0')
               << std::setw(2) << index << "log";
        return stream.str();
    }

    void open_latest_file_for_day(const std::tm &tm) {
        current_tm_ = tm;

        std::size_t candidate_index = 0;
        std::size_t last_existing_index = 0;
        bool found_existing = false;

        for (;; ++candidate_index) {
            auto candidate_name = make_filename(current_tm_, candidate_index);
            if (!spdlog::details::os::path_exists(candidate_name)) {
                break;
            }

            found_existing = true;
            last_existing_index = candidate_index;
        }

        file_index_ = found_existing ? last_existing_index : 0;
        open_file(make_filename(current_tm_, file_index_));

        if (current_size_ >= max_size_bytes_) {
            rotate_file();
        }
    }

    void rotate_file() {
        while (true) {
            ++file_index_;
            const auto filename = make_filename(current_tm_, file_index_);
            open_file(filename);

            if (current_size_ < max_size_bytes_) {
                break;
            }
        }
    }

    void open_file(const spdlog::filename_t &filename) {
        file_helper_.open(filename, false);
        current_size_ = file_helper_.size();
    }

    spdlog::details::file_helper file_helper_{};
    std::tm current_tm_{};
    std::size_t file_index_{0};
    std::size_t current_size_{0};
    std::size_t max_size_bytes_;
};

constexpr std::size_t kMaxLogFileSize = 10 * 1024 * 1024;

} // namespace

void LoggerInit(void)
{
    auto file_sink = std::make_shared<daily_size_file_sink_mt>(kMaxLogFileSize);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info); // ログレベル指定
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e, %-5l, %v");

    // [thread %t]で[thread
    // 32224]のようにスレッドIDが出る(とりあえずいらないので除外)
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-5l] [thread %t] %v");
}

void log_debug_impl(const char *file, int line, const char *fmtstr, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmtstr);
    vsnprintf(buf, sizeof(buf), fmtstr, args);
    va_end(args);

    std::string newmsg = std::string(buf) + " [" + file + ", " + std::to_string(line) + "]";
    spdlog::debug(newmsg);
}

void log_info_impl(const char *file, int line, const char *fmtstr, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmtstr);
    vsnprintf(buf, sizeof(buf), fmtstr, args);
    va_end(args);

    std::string newmsg = std::string(buf) + " [" + file + ", " + std::to_string(line) + "]";
    spdlog::info(newmsg);
}

void log_error_impl(const char *file, int line, const char *fmtstr, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmtstr);
    vsnprintf(buf, sizeof(buf), fmtstr, args);
    va_end(args);

    std::string newmsg = std::string(buf) + " [" + file + ", " + std::to_string(line) + "]";
    spdlog::error(newmsg);
}

} // namespace app
