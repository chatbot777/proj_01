#ifndef TOOL_LOGGER_H
#define TOOL_LOGGER_H

namespace app {

#define LOG_DEBUG(fmtstr, ...) log_debug_impl(__FILE__, __LINE__, fmtstr, ##__VA_ARGS__)
#define LOG_INFO(fmtstr, ...) log_info_impl(__FILE__, __LINE__, fmtstr, ##__VA_ARGS__)
#define LOG_ERROR(fmtstr, ...) log_error_impl(__FILE__, __LINE__, fmtstr, ##__VA_ARGS__)

void LoggerInit(void);
void log_debug_impl(const char *file, int line, const char *fmtstr, ...);
void log_info_impl(const char *file, int line, const char *fmtstr, ...);
void log_error_impl(const char *file, int line, const char *fmtstr, ...);

} // namespace app

#endif // TOOL_LOGGER_H
