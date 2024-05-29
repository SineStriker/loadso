#ifndef LOADSO_LIBRARY_H
#define LOADSO_LIBRARY_H

#include <memory>

#include <loadso/loadso_global.h>

namespace LoadSO {

    class LOADSO_EXPORT Library {
    public:
        Library();
        ~Library();

        Library(Library &&other) noexcept;
        Library &operator=(Library &&other) noexcept;

    public:
        /**
         * @brief Call `open` with a combination of the following hints.
         *
         */
        enum LoadHint {
            ResolveAllSymbolsHint = 0x01,
            ExportExternalSymbolsHint = 0x02,
            LoadArchiveMemberHint = 0x04, // Unused
            PreventUnloadHint = 0x08,
            DeepBindHint = 0x10
        };

        /**
         * @brief Load a library with a path, evaluated relative to the executable path if the path
         * is relative. If you're going to load another library, close the current one first.
         *
         * @param path Library path
         * @param hints Loading hints
         */
        bool open(const PathString &path, int hints = 0);

        /**
         * @brief Release the library.
         *
         */
        bool close();

        /**
         * @brief Whether the library is loaded.
         *
         */
        bool isOpen() const;

        /**
         * @return Library path
         */
        PathString path() const;

        /**
         * @brief Library handle
         *
         */
        DllHandle handle() const;

        /**
         * @brief Resolve a function by the name.
         *
         * @param name Function name
         */
        EntryHandle resolve(const std::string &name);

        /**
         * @brief System last error.
         *
         * @param nativeLanguage Whether to use native language, only works on Windows
         * @return Last error message if an operation fails
         */
        PathString lastError(bool nativeLanguage = false) const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}

#endif // LOADSO_LIBRARY_H
