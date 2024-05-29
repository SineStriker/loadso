#include <iostream>

#include <loadso/pluginloader.h>

#include "interface.h"

int main(int argc, char *argv[]) {
    LoadSO::PluginLoader plugin1(LOADSO_STR(PLUGIN1_NAME));
    LoadSO::PluginLoader plugin2(LOADSO_STR(PLUGIN2_NAME));

    // Get Metadata
    const auto &metadata1 = plugin1.metaData();
    if (metadata1.empty()) {
        printf("plugin1 get metadata failed\n");
    }
    printf("plugin1 metadata: %s\n", metadata1.data());

    const auto &metadata2 = plugin2.metaData();
    if (metadata2.empty()) {
        printf("plugin2 get metadata failed\n");
    }
    printf("plugin2 metadata: %s\n", plugin2.metaData().data());

    // Load
    if (!plugin1.load(LoadSO::Library::ResolveAllSymbolsHint)) {
        printf("plugin1 load failed\n");
        return -1;
    }

    if (!plugin2.load(LoadSO::Library::ResolveAllSymbolsHint)) {
        printf("plugin2 load failed\n");
        return -1;
    }

    // Get Instance
    auto instance1 = static_cast<LoadSO::Interface *>(plugin1.instance());
    if (!instance1) {
        printf("plugin2 get instance failed\n");
        return -1;
    }

    auto instance2 = static_cast<LoadSO::Interface *>(plugin2.instance());
    if (!instance2) {
        printf("plugin2 get instance failed\n");
        return -1;
    }

    printf("plugin1 key: %s\n", instance1->key());
    printf("plugin2 key: %s\n", instance2->key());

    return 0;
}