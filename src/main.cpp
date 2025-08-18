#include "tool_logger.h"
#include "sub.h"

int main()
{
    LoggerInit();
    LOG_INFO("Hello, world.");
    LOG_INFO("10 - 3 = %d", sub(10, 3));
    return 0;
}
