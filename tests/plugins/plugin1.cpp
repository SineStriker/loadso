#include "plugin1.h"

namespace LoadSO {

    Plugin::Plugin() {
        std::cout << "plugin1 constructs" << std::endl;
    }

    const char *Plugin::key() const {
        return "plugin1";
    }

}