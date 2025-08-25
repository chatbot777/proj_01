#include <thread>

#include "tool_logger.h"
#include "main_thread.h"
#include "simulation_thread.h"

namespace app
{

void MainThread(void)
{
    LOG_INFO("Main thread start");

    std::thread statusThread(StatusThread);
    statusThread.join();

    LOG_INFO("Main thread end");
}

} // namespace app
