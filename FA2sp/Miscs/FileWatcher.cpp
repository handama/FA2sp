#include "FileWatcher.h"

#include "../Ext/CFinalSunApp/Body.h"
#include "../Ext/CFinalSunDlg/Body.h"

#include "../FA2sp.h"

#include "../Helpers/Translations.h"

#include <thread>
#include <CIsoView.h>

bool FileWatcher::IsSavingMap = false;
bool FileWatcher::IsMapJustSaved = false;
bool FileWatcher::IsMessageBoxShowing = false;

FileWatcher::FileWatcher(const char* path, std::chrono::milliseconds delay, bool& running,
    std::optional<std::filesystem::file_time_type>& savemap_time)
    : path_{ path }
    , delay_{ delay }
    , running_{ running }
    , savemap_time_{ savemap_time }
{
    if (std::filesystem::exists(std::filesystem::path{path}))
    {
        previous_path_ = path;
        time_.emplace(std::filesystem::last_write_time(previous_path_));
    }
}

void FileWatcher::Callback(std::string path, Status status)
{
    UNREFERENCED_PARAMETER(path);

    if (ExtConfigs::IsQuitingProgram)
        return;

    if (status == Status::Modified)
    {
        const FString message = Translations::TranslateOrDefault(
            "FileWatcherMessage", "Mapfile has been modified externally. Please reload the map file to continue."
        );
        IsMessageBoxShowing = true;
        MessageBox(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd,
            message,
            "FA2sp",
            MB_OK | MB_ICONEXCLAMATION
        );
        IsMessageBoxShowing = false;
    }
}

void FileWatcher::start(const std::function<void(std::string, Status)>& action)
{
    while (running_)
    {
        std::this_thread::sleep_for(delay_);

        if (!ExtConfigs::FileWatcher)
            continue;
        
        if (IsSavingMap)
            continue;     

        if (ExtConfigs::IsQuitingProgram)
            continue;

        if (IsMapJustSaved)
        {
            IsMapJustSaved = false;
            continue;
        }

        const std::string curr_path = path_;
        if (curr_path != previous_path_)
        {
            if (std::filesystem::exists(std::filesystem::path{curr_path}))
            {
                previous_path_ = curr_path;
                time_.emplace(std::filesystem::last_write_time(previous_path_));
            }
            continue;
        }

        if (savemap_time_.has_value())
        {
            time_ = savemap_time_;
            savemap_time_.reset();
            continue;
        }

        if (!std::filesystem::exists(previous_path_) && time_.has_value())
        {
            action(previous_path_.string(), Status::Deleted);
            time_.reset();
        }
        else if (std::filesystem::exists(previous_path_))
        {
            if (!time_.has_value())
            {
                action(previous_path_.string(), Status::Created);
                time_.emplace(std::filesystem::last_write_time(previous_path_));
            }
            else
            {
                const auto current_time = std::filesystem::last_write_time(previous_path_);
                if (time_ != current_time)
                {
                    action(previous_path_.string(), Status::Modified);
                    time_.emplace(current_time);
                }
            }
        }
    }
}
