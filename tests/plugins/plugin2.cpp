#include <iostream>

#include "interface.h"

namespace LoadSO {

    class Plugin : public LoadSO::Interface {
    public:
        Plugin();

        const char *key() const override;
    };

    Plugin::Plugin() {
        std::cout << "plugin2 constructs" << std::endl;
    }

    const char *Plugin::key() const {
        return "plugin2";
    }

}

#include LOADSO_PLUGIN_SOURCE_FILE