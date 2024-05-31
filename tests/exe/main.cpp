#include <iostream>

#include <loadso/library.h>
#include <loadso/system.h>

#ifdef _WIN32
#  include <fcntl.h>
#  include <io.h>

struct LocaleGuard {
    LocaleGuard() {
        mode = _setmode(_fileno(stdout), _O_U16TEXT);
    }
    ~LocaleGuard() {
        _setmode(_fileno(stdout), mode);
    }
    int mode;
};
#endif

using namespace LoadSO;

void PrintLine(const PathString &text) {
#ifdef _WIN32
    LocaleGuard guard;
    std::wcout << text << std::endl;
#else
    std::cout << text << std::endl;
#endif
}

int main(int argc, char *argv[]) {
    PrintLine(LOADSO_STR("[Test Path]"));
    PrintLine(LOADSO_STR("App Path       : ") + System::ApplicationPath());
    PrintLine(LOADSO_STR("App File Name  : ") + System::ApplicationFileName());
    PrintLine(LOADSO_STR("App Directory  : ") + System::ApplicationDirectory());
    PrintLine(LOADSO_STR("App Name       : ") + System::ApplicationName());

    // Load library
    PrintLine(LOADSO_STR("[Test Load Library]"));
    Library lib;

    if (!lib.open(LOADSO_STR(DLL_NAME), Library::ResolveAllSymbolsHint)) {
        System::ShowError(System::MultiToPathString(lib.lastError()));
        return -1;
    }
    PrintLine(LOADSO_STR("OK"));

    // Get function
    PrintLine(LOADSO_STR("[Test Get Function]"));
    using AddFunc = int (*)(int, int);
    auto add_func = (AddFunc) lib.resolve("add");
    if (!add_func) {
        System::ShowError(System::MultiToPathString(lib.lastError()));
        return -1;
    }
    PrintLine(LOADSO_STR("OK"));

    // Call function
    std::cout << add_func(1, 3) << std::endl;

    return 0;
}