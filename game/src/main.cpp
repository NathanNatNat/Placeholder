#include "core/logging.h"

#include <spdlog/spdlog.h>

int main()
{
    placeholder::core::initLogging();
    spdlog::info("Placeholder Engine v0.1");
    return 0;
}
