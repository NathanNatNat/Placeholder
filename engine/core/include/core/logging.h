#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace placeholder::core
{

/// Initialize all engine loggers. Call once at startup.
void initLogging();

/// Retrieve a named subsystem logger.
std::shared_ptr<spdlog::logger> getLogger(const std::string& name);

} // namespace placeholder::core
