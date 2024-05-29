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

        /**
         * @param func Any static or global function in shared library
         * @return Absolute path of the shared library.
         */
        static PathString LibraryPath(EntryHandle &func);

        /**
         * Call SetDllDirectory on Windows, change LD_LIBRARY_PATH env on Unix.
         *
         * @param path Dll directory.
         * @return Previous library path.
         */
        static PathString SetLibraryPath(const PathString &path);

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

    public:
        static void ShowError(const PathString &text);
    };

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
