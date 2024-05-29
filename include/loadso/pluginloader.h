#ifndef LOADSO_PLUGINLOADER_H
#define LOADSO_PLUGINLOADER_H

#include <loadso/library.h>

namespace LoadSO {

    class LOADSO_EXPORT PluginLoader {
    public:
        PluginLoader(const PathString &path = {});
        ~PluginLoader();

        PluginLoader(PluginLoader &&other) noexcept;
        PluginLoader &operator=(PluginLoader &&other) noexcept;

    public:
        void *instance() const;
        const std::string &metaData() const;

        bool load(int hints);
        bool unload();
        bool isLoaded() const;

        PathString fileName() const;
        void setFileName(const PathString &fileName);

        PathString lastError(bool nativeLanguage = false) const;

    public:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}

#endif // LOADSO_PLUGINLOADER_H
