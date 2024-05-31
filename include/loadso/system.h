#ifndef LOADSO_SYSTEM_H
#define LOADSO_SYSTEM_H

#include <loadso/loadso_global.h>

namespace LoadSO {

    class LOADSO_EXPORT System {
    public:
        static PathString ApplicationFileName();
        static PathString ApplicationDirectory();
        static PathString ApplicationPath();
        static PathString ApplicationName();

#ifdef LOADSO_STD_FILESYSTEM
        static inline std::filesystem::path ApplicationFileName2();
        static inline std::filesystem::path ApplicationDirectory2();
        static inline std::filesystem::path ApplicationPath2();
        static inline std::filesystem::path ApplicationName2();
#endif

        /**
         * @param func Any static or global function in shared library
         * @return Absolute path of the shared library.
         */
        static PathString LibraryPath(EntryHandle &func);

#ifdef LOADSO_STD_FILESYSTEM
        static inline std::filesystem::path LibraryPath2(EntryHandle &func);
#endif

        /**
         * Call SetDllDirectory on Windows, change LD_LIBRARY_PATH env on Unix.
         *
         * @param path Dll directory.
         * @return Previous library path.
         */
        static PathString SetLibraryPath(const PathString &path);

#ifdef LOADSO_STD_FILESYSTEM
        static inline std::filesystem::path SetLibraryPath2(const std::filesystem::path &path);
#endif

    public:
        static bool IsRelativePath(const PathString &path);
        static PathString PathToNativeSeparator(const PathString &path);
        static PathString PathFromNativeSeparator(const PathString &path);

        /**
         * @param bytes Multi bytes String
         * @return Wide char string on Windows, the original string on Unix.
         */
        static PathString MultiToPathString(const std::string &bytes);

        /**
         * @param str Path string
         * @return Multi bytes string on Windows, the original string on Unix.
         */
        static std::string MultiFromPathString(const PathString &str);

        static void ShowError(const PathString &text);

#ifdef LOADSO_STD_FILESYSTEM
        static inline void ShowError2(const std::filesystem::path &text);
#endif
    };

#ifdef LOADSO_STD_FILESYSTEM
    inline std::filesystem::path System::ApplicationFileName2() {
        return ApplicationFileName();
    }

    inline std::filesystem::path System::ApplicationDirectory2() {
        return ApplicationDirectory();
    }

    inline std::filesystem::path System::ApplicationPath2() {
        return ApplicationPath();
    }

    inline std::filesystem::path System::ApplicationName2() {
        return ApplicationName2();
    }

    inline std::filesystem::path System::LibraryPath2(EntryHandle &func) {
        return LibraryPath(func);
    }

    inline std::filesystem::path System::SetLibraryPath2(const std::filesystem::path &path) {
        return SetLibraryPath(path);
    }

    inline void System::ShowError2(const std::filesystem::path &text) {
        ShowError(text);
    }
#endif

#ifndef _WIN32
    inline PathString System::MultiToPathString(const std::string &bytes) {
        return bytes;
    }

    inline std::string System::MultiFromPathString(const PathString &str) {
        return str;
    }
#endif

}

#endif // LOADSO_SYSTEM_H
