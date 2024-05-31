#ifndef LOADSO_PLUGINLOADER_H
#define LOADSO_PLUGINLOADER_H

#include <loadso/library.h>

namespace LoadSO {

    class LOADSO_EXPORT PluginLoader {
    public:
        explicit PluginLoader(const PathString &path = {});
        ~PluginLoader();

        PluginLoader(PluginLoader &&other) noexcept;
        PluginLoader &operator=(PluginLoader &&other) noexcept;

    public:
        /**
         * @brief Returns the plugin instance.
         *
         * @return Instance handle
         */
        void *instance() const;

        /**
         * @brief Returns the meta data for this plugin.
         *
         * @return Metadata byte array
         */
        const std::string &metaData() const;

        bool load(int hints);
        bool unload();
        bool isLoaded() const;

        PathString path() const;
        void setPath(const PathString &path);

#ifdef LOADSO_STD_FILESYSTEM
        inline std::filesystem::path path2() const;
        inline void setPath2(const std::filesystem::path &path);
#endif

        std::string lastError(bool nativeLanguage = false) const;

    public:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

#ifdef LOADSO_STD_FILESYSTEM
    inline std::filesystem::path PluginLoader::path2() const {
        return path();
    }

    inline void PluginLoader::setPath2(const std::filesystem::path &path) {
        setPath(path);
    }
#endif

}

#endif // LOADSO_PLUGINLOADER_H
