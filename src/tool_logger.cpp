#include <cstdarg>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "tool_logger.h"

namespace app {

void LoggerInit(void)
{
    // ファイル出力(毎日0時にローテート)
    auto file_sink =
        std::make_shared<spdlog::sinks::daily_file_format_sink_mt>("log/log_%Y_%m_%d.log", 0, 0);
    // コンソール出力
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
