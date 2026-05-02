#include "core/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace placeholder::core
{

static bool s_initialized = false;

void initLogging()
{
    if (s_initialized)
    {
        return;
    }

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("placeholder.log", true);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};

    const char* subsystems[] = {
        "core", "platform", "resources", "renderer",
        "input", "audio", "animation", "editor", "wowlib"
    };

    for (const auto* name : subsystems)
    {
        auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
    }

    spdlog::set_default_logger(spdlog::get("core"));
    spdlog::set_level(spdlog::level::trace);

    spdlog::get("core")->info("Logging initialized");

    s_initialized = true;
}

std::shared_ptr<spdlog::logger> getLogger(const std::string& name)
{
    auto logger = spdlog::get(name);
    if (!logger)
    {
        logger = spdlog::get("core");
    }
    return logger;
}

} // namespace placeholder::core
