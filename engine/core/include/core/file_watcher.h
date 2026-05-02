#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

namespace placeholder::core
{

/// Type of file change detected by the watcher.
enum class FileChangeType
{
    Modified,
    Created,
    Deleted,
    Renamed
};

/// A single file change event.
struct FileChangeEvent
{
    FileChangeType type;
    std::filesystem::path path;
};

/// Callback type for file change notifications.
using FileChangeCallback = std::function<void(const FileChangeEvent&)>;

/// Watches a directory for file changes using Win32 ReadDirectoryChangesW.
///
/// Runs a dedicated background thread. Changes are delivered via callback
/// on the watcher thread — consumers must synchronize if needed.
class FileWatcher
{
public:
    FileWatcher() = default;
    ~FileWatcher();

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /// Start watching a directory. Calls callback on the watcher thread.
    /// @param directory The directory to watch (must exist).
    /// @param callback Called for each detected change.
    /// @param recursive Watch subdirectories as well.
    /// @return true if the watcher started successfully.
    bool start(const std::filesystem::path& directory,
               FileChangeCallback callback,
               bool recursive = true);

    /// Stop watching and join the background thread.
    void stop();

    /// Whether the watcher is currently running.
    bool isRunning() const { return m_running.load(); }

private:
    void watchThread();

    std::filesystem::path m_directory;
    FileChangeCallback m_callback;
    bool m_recursive = true;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    void* m_directoryHandle = nullptr;
    void* m_stopEvent = nullptr;
};

} // namespace placeholder::core
