#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <filesystem>
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

std::string shell_escape(const std::string &input)
{
    std::string escaped;
    escaped.reserve(input.size() + 2);
    escaped.push_back('\'');
    for (char c : input) {
        if (c == '\'') {
            escaped += "'\\''";
        }
        else {
            escaped.push_back(c);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

void compress_file(const spdlog::filename_t &filename)
{
    if (filename.empty()) {
        return;
    }

    if (!spdlog::details::os::path_exists(filename)) {
        return;
    }

    spdlog::filename_t archive_filename = filename;
    archive_filename += SPDLOG_FILENAME_T(".tar.xz");
    spdlog::details::os::remove_if_exists(archive_filename);

    const auto file_path = std::filesystem::path(spdlog::details::os::filename_to_str(filename));
    const auto archive_path = std::filesystem::path(spdlog::details::os::filename_to_str(archive_filename));
    auto parent_path = file_path.parent_path();
    if (parent_path.empty()) {
        parent_path = std::filesystem::path(".");
    }

    const auto archive_str = archive_path.string();
    const auto parent_str = parent_path.string();
    const auto file_str = file_path.filename().string();

    std::ostringstream command;
    command << "tar -cJf " << shell_escape(archive_str) << " -C " << shell_escape(parent_str) << " "
            << shell_escape(file_str);
    const auto command_str = command.str();

    const int result = std::system(command_str.c_str());
    if (result != 0) {
        throw spdlog::spdlog_ex("Failed to compress log file: " + archive_str);
    }

    spdlog::details::os::remove_if_exists(filename);
}

// 1日の間に生成されるログファイルをサイズ上限で区切りながら出力するカスタムシンク
// (spdlogのファイルシンクを拡張して、日付ごと・ファイルサイズごとにローテーションする)
class daily_size_file_sink_mt final : public spdlog::sinks::base_sink<std::mutex>
{
public:
    explicit daily_size_file_sink_mt(std::size_t max_size_bytes) : max_size_bytes_(max_size_bytes)
    {
        if (max_size_bytes_ == 0) {
            throw spdlog::spdlog_ex("max_size_bytes must be greater than zero");
        }

        // 現在時刻に対応するファイルを開いてロギングを開始する
        open_latest_file_for_day(to_local_tm(spdlog::log_clock::now()));
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        const auto message_tm = to_local_tm(msg.time);
        if (!is_same_day(message_tm, current_tm_)) {
            // ログの日付が切り替わった場合は新しい日付のファイルへ切り替える
            open_latest_file_for_day(message_tm);
        }

        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);

        if (formatted.size() >= max_size_bytes_) {
            // 1メッセージでサイズ上限を超える場合は空き容量ができるまでファイルを進める
            while (current_size_ != 0) { rotate_file(); }
        }
        else {
            // 既存ファイルに追記できない場合は十分な空きがあるファイルが見つかるまでローテーション
            while (current_size_ + formatted.size() > max_size_bytes_) { rotate_file(); }
        }

        // 実際にフォーマット済みメッセージを書き込み、サイズを更新する
        file_helper_.write(formatted);
        current_size_ += formatted.size();
    }

    void flush_() override { file_helper_.flush(); }

private:
    static std::tm to_local_tm(spdlog::log_clock::time_point tp)
    {
        const auto time = spdlog::log_clock::to_time_t(tp);
        return spdlog::details::os::localtime(time);
    }

    static bool is_same_day(const std::tm &lhs, const std::tm &rhs)
    {
        return lhs.tm_year == rhs.tm_year && lhs.tm_yday == rhs.tm_yday;
    }

    static spdlog::filename_t make_filename(const std::tm &tm, std::size_t index)
    {
        // ファイル名は「log_YYYY_MM_DD._XXlog」という形式で連番を持つ
        std::ostringstream stream;
        stream << "log/log_" << std::put_time(&tm, "%Y_%m_%d") << "_" << std::setfill('0') << std::setw(2) << index
               << "log";
        return stream.str();
    }

    void open_latest_file_for_day(const std::tm &tm)
    {
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

        // 既存ファイルがあれば最後のものを再利用し、なければインデックス0から開始する
        file_index_ = found_existing ? last_existing_index : 0;
        open_file(make_filename(current_tm_, file_index_), has_current_file_);

        if (current_size_ >= max_size_bytes_) {
            // 既存ファイルがすでに上限を超えていれば即座に次のファイルにローテーション
            rotate_file();
        }
    }

    void rotate_file()
    {
        while (true) {
            ++file_index_;
            const auto filename = make_filename(current_tm_, file_index_);
            open_file(filename, true);

            if (current_size_ < max_size_bytes_) {
                // 書き込み可能なファイルを見つけたらローテーション完了
                break;
            }
        }
    }

    void open_file(const spdlog::filename_t &filename, bool finalize_previous)
    {
        if (finalize_previous && has_current_file_) {
            finalize_current_file();
        }

        // 指定されたファイルを開き、現在のサイズを記録する
        file_helper_.open(filename, false);
        current_filename_ = filename;
        current_size_ = file_helper_.size();
        has_current_file_ = true;
    }

    void finalize_current_file()
    {
        if (!has_current_file_) {
            return;
        }

        const auto previous_filename = current_filename_;
        file_helper_.flush();
        file_helper_.close();
        has_current_file_ = false;
        current_filename_.clear();
        current_size_ = 0;

        compress_file(previous_filename);
    }

    spdlog::details::file_helper file_helper_{};
    spdlog::filename_t current_filename_{};
    std::tm current_tm_{};
    std::size_t file_index_{0};
    std::size_t current_size_{0};
    std::size_t max_size_bytes_;
    bool has_current_file_{false};
};

constexpr std::size_t MAX_LOG_FILE_SIZE = 10 * 1024 * 1024;

} // namespace

void LoggerInit(void)
{
    // ファイルへの出力（本クラス）とコンソール出力を組み合わせた複数シンクのロガーを構築
    auto file_sink = std::make_shared<daily_size_file_sink_mt>(MAX_LOG_FILE_SIZE);
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

    // 可変長引数から成形したメッセージに、呼び出し元のファイル名と行番号を付加して出力
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

    // 共通処理：ログメッセージに呼び出し元情報を追記して出力
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

    // エラーログも同様に呼び出し元情報付きで出力する
    std::string newmsg = std::string(buf) + " [" + file + ", " + std::to_string(line) + "]";
    spdlog::error(newmsg);
}

} // namespace app
