#ifndef PLUGINLOADER_P_H
#define PLUGINLOADER_P_H

#include "pluginloader.h"
#include "library_p.h"

namespace LoadSO {

    class PluginLoader::Impl : public Library::Impl {
    public:
        void *pluginInstance = nullptr;

        mutable std::string metaData;
        mutable bool metaDataLoaded = false;

        void getMetaData() const;
    };

}

#endif // PLUGINLOADER_P_H
