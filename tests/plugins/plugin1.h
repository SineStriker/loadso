#ifndef PLUGIN1_H
#define PLUGIN1_H

#include <iostream>

#include "interface.h"

namespace LoadSO {

    class Plugin : public LoadSO::Interface {
    public:
        Plugin();

        const char *key() const override;
    };

}

#endif // PLUGIN1_H
