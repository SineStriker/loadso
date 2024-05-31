#ifndef LOADSO_LIBRARY_H
#define LOADSO_LIBRARY_H

#include <memory>

#include <loadso/loadso_global.h>

namespace LoadSO {

    class PluginLoader;

    class LOADSO_EXPORT Library {
    public:
        Library();
        ~Library();

        Library(Library &&other) noexcept;
        Library &operator=(Library &&other) noexcept;

    public:
        /**
         * @brief Give the open() function some hints on how it should behave.
         */
        enum LoadHint {
            ResolveAllSymbolsHint = 0x01,
            ExportExternalSymbolsHint = 0x02,
            LoadArchiveMemberHint = 0x04, // Unused
            PreventUnloadHint = 0x08,
            DeepBindHint = 0x10
        };

        /**
         * @brief Loads a library with a path, evaluated relative to the executable path if the path
         *        is relative. If you're going to load another library, close the current one first.
         *
         * @param path Library path
         * @param hints Loading hints
         */
        bool open(const PathString &path, int hints = 0);

#ifdef LOADSO_STD_FILESYSTEM
        inline bool open2(const std::filesystem::path &path, int hints = 0);
#endif

        /**
         * @brief Frees the library and returns \c true if successful.
         */
        bool close();

        /**
         * @brief Returns \c true if the library is loaded.
         */
        bool isOpen() const;

        /**
         * @brief Returns the library path.
         */
        PathString path() const;

#ifdef LOADSO_STD_FILESYSTEM
        inline std::filesystem::path path2() const;
#endif

        /**
         * @brief Returns the ibrary handle.
         */
        LibraryHandle handle() const;

        /**
         * @brief Returns the address of the exported symbol by name, the library
         *        should be loaded first.
         *
         * @param name Function name
         */
        EntryHandle resolve(const char *name) const;

        /**
         * @brief Returns the system's last error.
         *
         * @param nativeLanguage Whether to use native language, only works on Windows
         * @return Last error message if an operation fails, UTF-8 encoded
         */
        std::string lastError(bool nativeLanguage = false) const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

        friend class PluginLoader;
    };

#ifdef LOADSO_STD_FILESYSTEM
    inline bool Library::open2(const std::filesystem::path &path, int hints) {
        return open(path, hints);
    }

    inline std::filesystem::path Library::path2() const {
        return path();
    }
#endif

}

#endif // LOADSO_LIBRARY_H
