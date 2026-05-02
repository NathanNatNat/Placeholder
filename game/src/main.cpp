#include "core/logging.h"
#include "core/config.h"

#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    placeholder::core::initLogging();
    spdlog::info("Placeholder Engine v0.1");

    placeholder::core::Config config;
    config.loadFromFile(PLACEHOLDER_ROOT_DIR "/assets/config/engine.json");
    config.applyCommandLine(argc, argv);

    spdlog::info("Window: {}x{} \"{}\"",
        config.get<int>("window.width"),
        config.get<int>("window.height"),
        config.get<std::string>("window.title"));

    return 0;
}
