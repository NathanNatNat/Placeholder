#include "core/file_watcher.h"
#include "core/logging.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace placeholder::core
{

FileWatcher::~FileWatcher()
{
    stop();
}

bool FileWatcher::start(const std::filesystem::path& directory,
                        FileChangeCallback callback,
                        bool recursive)
{
    if (m_running.load())
    {
        return false;
    }

    auto log = getLogger("core");

    if (!std::filesystem::is_directory(directory))
    {
        log->error("FileWatcher: not a directory: {}", directory.string());
        return false;
    }

    m_directoryHandle = CreateFileW(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (m_directoryHandle == INVALID_HANDLE_VALUE)
    {
        log->error("FileWatcher: failed to open directory: {}", directory.string());
        m_directoryHandle = nullptr;
        return false;
    }

    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent)
    {
        CloseHandle(m_directoryHandle);
        m_directoryHandle = nullptr;
        log->error("FileWatcher: failed to create stop event");
        return false;
    }

    m_directory = directory;
    m_callback = std::move(callback);
    m_recursive = recursive;
    m_running.store(true);

    m_thread = std::thread(&FileWatcher::watchThread, this);
    log->info("FileWatcher: watching {}", directory.string());

    return true;
}

void FileWatcher::stop()
{
    if (!m_running.load())
    {
        return;
    }

    m_running.store(false);

    if (m_stopEvent)
    {
        SetEvent(static_cast<HANDLE>(m_stopEvent));
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    if (m_directoryHandle)
    {
        CloseHandle(m_directoryHandle);
        m_directoryHandle = nullptr;
    }

    if (m_stopEvent)
    {
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }

    getLogger("core")->info("FileWatcher: stopped");
}

void FileWatcher::watchThread()
{
    auto log = getLogger("core");

    constexpr DWORD BUFFER_SIZE = 64 * 1024;
    auto buffer = std::make_unique<uint8_t[]>(BUFFER_SIZE);

    const DWORD filter =
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION;

    while (m_running.load())
    {
        OVERLAPPED overlapped{};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        BOOL result = ReadDirectoryChangesW(
            m_directoryHandle,
            buffer.get(),
            BUFFER_SIZE,
            m_recursive ? TRUE : FALSE,
            filter,
            nullptr,
            &overlapped,
            nullptr
        );

        if (!result)
        {
            CloseHandle(overlapped.hEvent);
            log->error("FileWatcher: ReadDirectoryChangesW failed");
            break;
        }

        HANDLE handles[2] = { overlapped.hEvent, static_cast<HANDLE>(m_stopEvent) };
        DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        CloseHandle(overlapped.hEvent);

        if (waitResult == WAIT_OBJECT_0 + 1 || !m_running.load())
        {
            CancelIo(m_directoryHandle);
            break;
        }

        if (waitResult != WAIT_OBJECT_0)
        {
            continue;
        }

        DWORD bytesTransferred = 0;
        GetOverlappedResult(m_directoryHandle, &overlapped, &bytesTransferred, FALSE);

        if (bytesTransferred == 0)
        {
            continue;
        }

        auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.get());
        while (true)
        {
            std::wstring fileName(info->FileName, info->FileNameLength / sizeof(wchar_t));

            FileChangeType changeType;
            switch (info->Action)
            {
            case FILE_ACTION_ADDED:
                changeType = FileChangeType::Created;
                break;
            case FILE_ACTION_REMOVED:
                changeType = FileChangeType::Deleted;
                break;
            case FILE_ACTION_MODIFIED:
                changeType = FileChangeType::Modified;
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
            case FILE_ACTION_RENAMED_NEW_NAME:
                changeType = FileChangeType::Renamed;
                break;
            default:
                changeType = FileChangeType::Modified;
                break;
            }

            FileChangeEvent event{changeType, m_directory / fileName};

            if (m_callback)
            {
                m_callback(event);
            }

            if (info->NextEntryOffset == 0)
            {
                break;
            }

            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<uint8_t*>(info) + info->NextEntryOffset
            );
        }
    }
}

} // namespace placeholder::core
