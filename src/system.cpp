#include "loadso/system.h"

#include <algorithm>
#include <cstring>

#ifdef _WIN32
#  include <Windows.h>
// 12345
#  include <Shlwapi.h>
#else
#  include <dlfcn.h>
#  include <limits.h>
#  ifdef __APPLE__
#    include <crt_externs.h>
#    include <mach-o/dyld.h>
#  endif
#endif

#ifdef __APPLE__
#  define PRIOR_LIBRARY_PATH_KEY "DYLD_LIBRARY_PATH"
#else
#  define PRIOR_LIBRARY_PATH_KEY "LD_LIBRARY_PATH"
#endif

namespace LoadSO {

#ifdef _WIN32

    static std::wstring winGetFullModuleFileName(HMODULE hModule) {
        // Use stack buffer for the first try
        wchar_t stackBuf[MAX_PATH + 1];

        // Call
        wchar_t *buf = stackBuf;
        auto size = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
        if (size == 0) {
            return {};
        }
        if (size > MAX_PATH) {
            // Re-alloc
            buf = new wchar_t[size + 1]; // The return size doesn't contain the terminating 0

            // Call
            if (::GetModuleFileNameW(nullptr, buf, size) == 0) {
                delete[] buf;
                return {};
            }
        }

        // std::replace(buf, buf + size, L'\\', L'/');

        // Return
        std::wstring res(buf);
        if (buf != stackBuf) {
            delete[] buf;
        }
        return res;
    }

    static std::wstring winGetFullDllDirectory() {
        auto size = ::GetDllDirectoryW(0, nullptr);
        if (!size)
            return {};

        auto buf = new wchar_t[size + 1];
        size = ::GetDllDirectoryW(size, buf);
        if (!size) {
            delete[] buf;
            return {};
        }

        std::wstring res(buf);
        delete[] buf;
        return res;
    }

#elif defined(__APPLE__)

    static std::string macGetExecutablePath() {
        // Use stack buffer for the first try
        char stackBuf[MAX_PATH + 1];

        // "_NSGetExecutablePath" will return "-1" if the buffer is not large enough
        // and "*bufferSize" will be set to the size required.

        // Call
        unsigned int size = MAX_PATH + 1;
        char *buf = stackBuf;
        if (_NSGetExecutablePath(buf, &size) != 0) {
            // Re-alloc
            buf = new char[size]; // The return size contains the terminating 0

            // Call
            if (_NSGetExecutablePath(buf, &size) != 0) {
                delete[] buf;
                return {};
            }
        }

        // Return
        std::string res(buf);
        if (buf != stackBuf) {
            delete[] buf;
        }
        return res;
    }

#endif

    PathString System::ApplicationFileName() {
        auto appName = ApplicationPath();
        auto slashIdx = appName.find_last_of(PathSeparator);
        if (slashIdx != std::string::npos) {
            appName = appName.substr(slashIdx + 1);
        }
        return appName;
    }

    PathString System::ApplicationDirectory() {
        auto appDir = ApplicationPath();
        auto slashIdx = appDir.find_last_of(PathSeparator);
        if (slashIdx != std::string::npos) {
            appDir = appDir.substr(0, slashIdx);
        }
        return appDir;
    }

    PathString System::ApplicationPath() {
        static const auto res = []() -> PathString {
#ifdef _WIN32
            return winGetFullModuleFileName(nullptr);
#elif defined(__APPLE__)
            return macGetExecutablePath();
#else
            char buf[PATH_MAX];
            if (!realpath("/proc/self/exe", buf)) {
                return {};
            }
            return buf;
#endif
        }();
        return res;
    }

    PathString System::ApplicationName() {
        auto appName = ApplicationFileName();
#ifdef _WIN32
        auto dotIdx = appName.find_last_of(L'.');
        if (dotIdx != PathString::npos) {
            auto suffix = appName.substr(dotIdx + 1);
            std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
            if (suffix == L"exe") {
                appName = appName.substr(0, dotIdx);
            }
        }
#endif
        return appName;
    }

    PathString System::LibraryPath(EntryHandle &func) {
#ifdef _WIN32
        HMODULE hModule = nullptr;
        if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                (LPCWSTR) &func, &hModule)) {
            return {};
        }
        return winGetFullModuleFileName(hModule);
#else
        Dl_info dl_info;
        dladdr((void *) func, &dl_info);
        auto buf = dl_info.dli_fname;
        return buf;
#endif
    }

    PathString System::SetLibraryPath(const PathString &path) {
#ifdef _WIN32
        std::wstring org = winGetFullDllDirectory();
        ::SetDllDirectoryW(path.data());
#else
        std::string org = getenv(PRIOR_LIBRARY_PATH_KEY);
        putenv((char *) (PRIOR_LIBRARY_PATH_KEY "=" + path).data());
#endif
        return org;
    }

    bool System::IsRelativePath(const PathString &path) {
#ifdef _WIN32
        return ::PathIsRelativeW(path.data());
#else
        auto p = path.data();
        while (p && *p == ' ') {
            p++;
        }
        return !p || *p != '/';
#endif
    }

    PathString System::PathToNativeSeparator(const PathString &path) {
        auto res = path;
        for (auto &ch : res) {
#ifdef _WIN32
            if (ch == L'/')
#else
            if (ch == '\\')
#endif
                ch = PathSeparator;
        }
        return res;
    }

    PathString System::PathFromNativeSeparator(const PathString &path) {
        auto res = path;
        for (auto &ch : res) {
            if (ch == L'\\')
                ch = L'/';
        }
        return res;
    }

    PathString System::MultiToPathString(const std::string &bytes) {
#ifdef _WIN32
        int len = ::MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int) bytes.size(), nullptr, 0);
        auto buf = new wchar_t[len + 1];
        ::MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int) bytes.size(), buf, len);
        buf[len] = '\0';

        std::wstring res(buf);
        delete[] buf;
        return res;
#else
        return bytes;
#endif
    }

    std::string System::MultiFromPathString(const PathString &str) {
#ifdef _WIN32
        int len = ::WideCharToMultiByte(CP_UTF8, 0, str.data(), (int) str.size(), nullptr, 0,
                                        nullptr, nullptr);
        auto buf = new char[len + 1];
        ::WideCharToMultiByte(CP_UTF8, 0, str.data(), (int) str.size(), buf, len, nullptr, nullptr);
        buf[len] = '\0';

        std::string res(buf);
        delete[] buf;
        return res;
#else
        return str;
#endif
    }

    void System::ShowError(const PathString &text) {
#ifdef _WIN32
        auto AppName = ApplicationName();
        ::MessageBoxW(nullptr, text.data(), AppName.empty() ? L"Fatal Error" : AppName.data(),
                      MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
#else
        fprintf(stderr, "%s\n", text.data());
#endif
    }

}