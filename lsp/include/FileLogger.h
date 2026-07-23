#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <filesystem>

namespace filelog {

    class Logger {
    public:
        Logger()
        {
            std::filesystem::path exe_path;
        
#if defined(_WIN32)
            wchar_t buffer[MAX_PATH];
            GetModuleFileNameW(NULL, buffer, MAX_PATH);
            exe_path = std::filesystem::path(buffer).parent_path();
#else
            exe_path = std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
        
            std::filesystem::path log_path = exe_path / "log.txt";
            log_file_.open(log_path, std::ios::out | std::ios::trunc);
        }

        ~Logger()
        {
            if (log_file_.is_open())
            {
                log_file_.close();
            }
        }

        void log(const std::string& message)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (log_file_.is_open())
            {
                log_file_ << timestamp() << " - " << message << "\n";
                log_file_.flush();
            }
        }

        static Logger& instance()
        {
            static Logger logger;
            return logger;
        }

    private:
        std::ofstream log_file_;
        std::mutex mutex_;

        std::string timestamp()
        {
            auto now = std::time(nullptr);
            std::tm buf{};
#if defined(_WIN32)
            localtime_s(&buf, &now);
#else
            localtime_r(&now, &buf);
#endif
            std::ostringstream ss;
            ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
            return ss.str();
        }
    };

    inline void LogMessage(const std::string& msg)
    {
        Logger::instance().log(msg);
    }

} // namespace filelog

#define LOG(msg) ::filelog::LogMessage(msg)