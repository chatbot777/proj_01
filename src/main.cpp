#include "tool_logger.h"

int main()
{
    // ロガーの初期化処理
    app::LoggerInit();
    app::LOG_INFO("main start");

    while (true) { continue; }

    app::LOG_INFO("main end");
    return 0;
}
