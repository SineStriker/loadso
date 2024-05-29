# LoadSO

Cross-platform class to handle shared library.

## Supported Platforms

+ Microsoft Windows
+ Apple Mac OSX
+ GNU/Linux (Tested on Ubuntu)

## Features

### Shared Library Utility

```c++
#include <filesystem>

#include <loadso/library.h>

int main(int argc, char *argv[]) {
    // Load libary
    LoadSO::Library lib;
    if (!lib.open(std::filesystem::path("add.dll"))) {
        return -1;
    }

    using AddFunc = int (*) (int, int);

    // Get function address
    auto func = reinterpret_cast<AddFunc>(lib.resolve("func"));
    if (!func) {
        return -1;
    }

    // Call
    std::cout << func(1, 2) << std::endl;
    return 0;
}
```

### Tiny Plugin Framework

+ plugin.txt
    ```
    some metadata
    ```

+ CMakeLists.txt
    ```c++
    # Plugin
    add_library(plugin plugin.h plugin.cpp)
    target_compile_features(plugin PRIVATE cxx_std_17)

    # Add plugin resource sources to the target 
    loadso_export_plugin(plugin
        plugin.h                    # header
        App::Plugin                 # class name
        METADATA_FILE plugin.txt    # metadata file
    )

    # Loader
    add_executable(loader loader.cpp)
    target_compile_features(loader PRIVATE cxx_std_17)
    target_link_libraries(loader PRIVATE loadso::loadso)
    ```

+ interface.h
    ```c++
    // Define some API as virtual functions
    class App {
        class Interface {
        public:
            virtual ~Interface() = default;
            virtual void someFeature() = 0;
        };
    }
    ```

+ plugin.h
    ```c++
    // Implement interface
    #include "interface.h"

    class App {
        class Plugin : public Interface {
        public:
            void someFeature() override;
        };
    }
    ```

+ loader.cpp
    ```c++
    #include <iostream>
    #include <filesystem>

    #include <loadso/pluginloader.h>

    #include "interface.h"

    int main(int argc, char *argv[]) {
        LoadSO::PluginLoader plugin;
        plugin.setPath(std::filesystem::path("plugin.dll"));

        // Check metadata before loading the library
        if (plugin.metaData() != "some metadata") {
            printf("This is not the plugin I want.\n");
            return -1;
        }

        // Load library
        if (!plugin.load(LoadSO::Library::ResolveAllSymbolsHint)) {
            printf("Load plugin failed\n");
            return -1;
        }

        // Get Instance
        auto instance = static_cast<App::Interface *>(plugin.instance());
        if (!instance) {
            printf("Get instance failed\n");
            return -1;
        };

        // Call feature
        instance->someFeature();
        return 0;
    }
    ```