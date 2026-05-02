#include "core/config.h"
#include "core/logging.h"

#include <fstream>
#include <sstream>

namespace placeholder::core
{

bool Config::loadFromFile(const std::filesystem::path& path)
{
    auto log = getLogger("core");

    std::ifstream file(path);
    if (!file.is_open())
    {
        log->error("Failed to open config file: {}", path.string());
        return false;
    }

    try
    {
        m_data = nlohmann::json::parse(file);
        m_filePath = path;
        log->info("Loaded config from {}", path.string());
        return true;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        log->error("Failed to parse config file {}: {}", path.string(), e.what());
        return false;
    }
}

bool Config::saveToFile(const std::filesystem::path& path) const
{
    auto log = getLogger("core");

    const auto& target = path.empty() ? m_filePath : path;
    if (target.empty())
    {
        log->error("No config file path specified for save");
        return false;
    }

    std::ofstream file(target);
    if (!file.is_open())
    {
        log->error("Failed to open config file for writing: {}", target.string());
        return false;
    }

    file << m_data.dump(4);
    log->info("Saved config to {}", target.string());
    return true;
}

void Config::applyCommandLine(int argc, char* argv[])
{
    auto log = getLogger("core");

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) != 0)
        {
            continue;
        }
        arg = arg.substr(2);

        auto eqPos = arg.find('=');
        if (eqPos == std::string::npos)
        {
            continue;
        }

        std::string key = arg.substr(0, eqPos);
        std::string value = arg.substr(eqPos + 1);

        auto& target = resolveOrCreate(key);

        if (target.is_boolean())
        {
            target = (value == "true" || value == "1");
        }
        else if (target.is_number_integer())
        {
            try { target = std::stoi(value); }
            catch (...) { log->warn("Invalid integer for --{}: {}", key, value); continue; }
        }
        else if (target.is_number_float())
        {
            try { target = std::stod(value); }
            catch (...) { log->warn("Invalid float for --{}: {}", key, value); continue; }
        }
        else
        {
            target = value;
        }

        log->info("Config override: {} = {}", key, target.dump());
    }
}

bool Config::has(const std::string& keyPath) const
{
    return resolve(keyPath) != nullptr;
}

const nlohmann::json* Config::resolve(const std::string& keyPath) const
{
    const nlohmann::json* current = &m_data;
    std::istringstream stream(keyPath);
    std::string segment;

    while (std::getline(stream, segment, '.'))
    {
        if (!current->is_object() || !current->contains(segment))
        {
            return nullptr;
        }
        current = &(*current)[segment];
    }

    return current;
}

nlohmann::json& Config::resolveOrCreate(const std::string& keyPath)
{
    nlohmann::json* current = &m_data;
    std::istringstream stream(keyPath);
    std::string segment;

    while (std::getline(stream, segment, '.'))
    {
        if (!current->is_object())
        {
            *current = nlohmann::json::object();
        }
        current = &(*current)[segment];
    }

    return *current;
}

} // namespace placeholder::core
