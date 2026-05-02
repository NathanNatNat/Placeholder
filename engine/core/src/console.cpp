#include "core/console.h"
#include "core/logging.h"

#include <algorithm>
#include <sstream>

namespace placeholder::core
{

void Console::registerCommand(const std::string& name,
                              const std::string& help,
                              CommandCallback callback)
{
    std::lock_guard lock(m_mutex);
    m_commands[name] = {name, help, std::move(callback)};
    getLogger("core")->debug("Console: registered command '{}'", name);
}

std::string Console::execute(const std::string& input)
{
    std::lock_guard lock(m_mutex);

    auto trimmed = input;
    while (!trimmed.empty() && trimmed.back() == ' ')
    {
        trimmed.pop_back();
    }
    while (!trimmed.empty() && trimmed.front() == ' ')
    {
        trimmed.erase(trimmed.begin());
    }

    if (trimmed.empty())
    {
        return {};
    }

    m_history.push_back(trimmed);
    if (m_history.size() > MAX_HISTORY)
    {
        m_history.erase(m_history.begin());
    }

    m_output.push_back("> " + trimmed);

    auto tokens = tokenize(trimmed);
    if (tokens.empty())
    {
        return {};
    }

    std::string commandName = tokens[0];
    std::transform(commandName.begin(), commandName.end(), commandName.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto it = m_commands.find(commandName);
    if (it == m_commands.end())
    {
        std::string error = "Unknown command: " + commandName;
        m_output.push_back(error);
        return error;
    }

    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    std::string result;

    try
    {
        result = it->second.callback(args);
    }
    catch (const std::exception& e)
    {
        result = std::string("Error: ") + e.what();
    }

    if (!result.empty())
    {
        m_output.push_back(result);
    }

    return result;
}

std::vector<std::string> Console::autocomplete(const std::string& prefix) const
{
    std::lock_guard lock(m_mutex);

    std::string lowerPrefix = prefix;
    std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::vector<std::string> matches;
    for (const auto& [name, entry] : m_commands)
    {
        if (name.rfind(lowerPrefix, 0) == 0)
        {
            matches.push_back(name);
        }
    }

    std::sort(matches.begin(), matches.end());
    return matches;
}

void Console::clearOutput()
{
    std::lock_guard lock(m_mutex);
    m_output.clear();
}

std::string Console::helpText(const std::string& commandName) const
{
    std::lock_guard lock(m_mutex);

    if (!commandName.empty())
    {
        auto it = m_commands.find(commandName);
        if (it != m_commands.end())
        {
            return it->second.name + " — " + it->second.help;
        }
        return "Unknown command: " + commandName;
    }

    std::vector<std::string> names;
    names.reserve(m_commands.size());
    for (const auto& [name, entry] : m_commands)
    {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());

    std::ostringstream ss;
    ss << "Available commands:\n";
    for (const auto& name : names)
    {
        const auto& entry = m_commands.at(name);
        ss << "  " << entry.name << " — " << entry.help << "\n";
    }

    return ss.str();
}

std::vector<std::string> Console::tokenize(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(std::move(current));
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        tokens.push_back(std::move(current));
    }

    return tokens;
}

} // namespace placeholder::core
