#include <iostream>

#include <loadso/library.h>
#include <loadso/system.h>

int main(int argc, char *argv[]) {
    LoadSO::System::PrintLine(LOADSO_STR("[Test Path]"));
    LoadSO::System::PrintLine(LOADSO_STR("App Path       : ") + LoadSO::System::ApplicationPath());
    LoadSO::System::PrintLine(LOADSO_STR("App File Name  : ") + LoadSO::System::ApplicationFileName());
    LoadSO::System::PrintLine(LOADSO_STR("App Directory  : ") + LoadSO::System::ApplicationDirectory());
    LoadSO::System::PrintLine(LOADSO_STR("App Name       : ") + LoadSO::System::ApplicationName());

    // Load library
    LoadSO::System::PrintLine(LOADSO_STR("[Test Load Library]"));
    LoadSO::Library lib;
    if (!lib.open(LOADSO_STR(
#ifndef _WIN32
        "../lib/"
#endif
        DLL_NAME
        ))) {
        LoadSO::System::ShowError(lib.lastError());
        return -1;
    }
    LoadSO::System::PrintLine(LOADSO_STR("OK"));

    // Get function
    LoadSO::System::PrintLine(LOADSO_STR("[Test Get Function]"));
    using AddFunc = int (*)(int, int);
    auto add_func = (AddFunc) lib.entry("add");
    if (!add_func) {
        LoadSO::System::ShowError(lib.lastError());
        return -1;
    }
    LoadSO::System::PrintLine(LOADSO_STR("OK"));

    // Call function
    std::cout << add_func(1, 3) << std::endl;

    return 0;
}