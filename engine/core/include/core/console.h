#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace placeholder::core
{

/// Developer console backend.
///
/// Supports command registration, parsing, dispatch, history, and output.
/// No rendering — the ImGui frontend comes in Phase 7.
class Console
{
public:
    /// Callback signature: receives the argument tokens, returns output text.
    using CommandCallback = std::function<std::string(const std::vector<std::string>&)>;

    /// Register a command with a name, help string, and callback.
    void registerCommand(const std::string& name,
                         const std::string& help,
                         CommandCallback callback);

    /// Execute a command string (e.g. "wireframe on").
    /// Tokenizes, dispatches, appends to history, and returns output.
    std::string execute(const std::string& input);

    /// Get registered command names that start with the given prefix (for autocomplete).
    std::vector<std::string> autocomplete(const std::string& prefix) const;

    /// Get the command history (most recent last).
    const std::vector<std::string>& history() const { return m_history; }

    /// Get the output log (all command inputs and outputs).
    const std::vector<std::string>& outputLog() const { return m_output; }

    /// Clear the output log.
    void clearOutput();

    /// Get help text for a specific command, or list all commands.
    std::string helpText(const std::string& commandName = "") const;

private:
    static std::vector<std::string> tokenize(const std::string& input);

    struct CommandEntry
    {
        std::string name;
        std::string help;
        CommandCallback callback;
    };

    std::unordered_map<std::string, CommandEntry> m_commands;

    std::vector<std::string> m_history;
    std::vector<std::string> m_output;
    mutable std::mutex m_mutex;

    static constexpr size_t MAX_HISTORY = 256;
};

} // namespace placeholder::core
