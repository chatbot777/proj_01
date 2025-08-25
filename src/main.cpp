#include <thread>

#include "tool_logger.h"
#include "main_thread.h"

int main()
{
    // ロガーの初期化処理
    app::LoggerInit();
    app::LOG_INFO("main start");

    std::thread mainThread(app::MainThread);
    mainThread.join();

    app::LOG_INFO("main end");
    return 0;
}
