#include "library.h"
#include "library_p.h"

#include <tuple>

#include "system.h"

#ifdef _WIN32
#  include <Windows.h>
// 12345
#  include <Shlwapi.h>
#else
#  include <dlfcn.h>
#  include <limits.h>
#  include <string.h>
#endif

namespace LoadSO {

#ifdef _WIN32

    static constexpr const DWORD g_EnglishLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    static std::wstring winErrorMessage(uint32_t error, bool nativeLanguage = true) {
        std::wstring rc;
        wchar_t *lpMsgBuf;

        const DWORD len =
            ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, error, nativeLanguage ? 0 : g_EnglishLangId,
                             reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, NULL);

        if (len) {
            // Remove tail line breaks
            if (lpMsgBuf[len - 1] == L'\n') {
                lpMsgBuf[len - 1] = L'\0';
                if (len > 2 && lpMsgBuf[len - 2] == L'\r') {
                    lpMsgBuf[len - 2] = L'\0';
                }
            }
            rc = std::wstring(lpMsgBuf, int(len));
            ::LocalFree(lpMsgBuf);
        } else {
            rc += L"unknown error";
        }

        return rc;
    }

#endif

    Library::Impl::~Impl() {
        std::ignore = close();
    }

    int Library::Impl::nativeLoadHints(int loadHints) {
#ifdef _WIN32
        return 0;
#else
        int dlFlags = 0;
        if (loadHints & Library::ResolveAllSymbolsHint) {
            dlFlags |= RTLD_NOW;
        } else {
            dlFlags |= RTLD_LAZY;
        }
        if (loadHints & ExportExternalSymbolsHint) {
            dlFlags |= RTLD_GLOBAL;
        }
#  if !defined(Q_OS_CYGWIN)
        else {
            dlFlags |= RTLD_LOCAL;
        }
#  endif
#  if defined(RTLD_DEEPBIND)
        if (loadHints & DeepBindHint)
            dlFlags |= RTLD_DEEPBIND;
#  endif
        return dlFlags;
#endif
    }

    PathString Library::Impl::sysErrorMessage(bool nativeLanguage) {
#ifdef _WIN32
        return winErrorMessage(::GetLastError(), nativeLanguage);
#else
        auto err = dlerror();
        if (err) {
            return err;
        }
        return {};
#endif
    }

    bool Library::Impl::open(int hints) {
        PathString absPath;
        if (System::IsRelativePath(path)) {
            absPath = System::ApplicationDirectory() + PathSeparator + path;
        } else {
            absPath = path;
        }

        auto handle =
#ifdef _WIN32
            ::LoadLibraryW(absPath.data())
#else
            dlopen(absPath.data(), Impl::nativeLoadHints(hints))
#endif
            ;
        if (!handle) {
            return false;
        }

#ifdef _WIN32
        if (hints & PreventUnloadHint) {
            // prevent the unloading of this component
            HMODULE hmod;
            ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN |
                                     GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                 reinterpret_cast<const wchar_t *>(handle), &hmod);
        }
#endif

        hDll = handle;
        return true;
    }

    bool Library::Impl::close() {
        if (!hDll) {
            return true;
        }

        if (!
#ifdef _WIN32
            ::FreeLibrary(reinterpret_cast<HMODULE>(hDll))
#else
            (dlclose(hDll) == 0)
#endif
        ) {
            return false;
        }

        hDll = nullptr;
        return true;
    }

    void *Library::Impl::resolve(const char *name) const {
        if (!hDll) {
            return nullptr;
        }

        auto addr =
#ifdef _WIN32
            ::GetProcAddress(reinterpret_cast<HMODULE>(hDll), name)
#else
            dlsym(hDll, name)
#endif
            ;
        return reinterpret_cast<void *>(addr);
    }

    Library::Library() : _impl(std::make_unique<Impl>()) {
    }

    Library::~Library() = default;

    Library::Library(Library &&other) noexcept {
        std::swap(_impl, other._impl);
    }

    Library &Library::operator=(Library &&other) noexcept {
        if (this == &other)
            return *this;
        std::swap(_impl, other._impl);
        return *this;
    }

    bool Library::open(const PathString &path, int hints) {
        _impl->path = path;
        if (_impl->open(hints)) {
            return true;
        }
        _impl->path.clear();
        return false;
    }

    bool Library::close() {
        if (_impl->close()) {
            _impl->path.clear();
            return true;
        }
        return false;
    }

    bool Library::isOpen() const {
        return _impl->hDll != nullptr;
    }

    PathString Library::path() const {
        return _impl->path;
    }

    DllHandle Library::handle() const {
        return _impl->hDll;
    }

    EntryHandle Library::resolve(const char *name) const {
        return _impl->resolve(name);
    }

    PathString Library::lastError(bool nativeLanguage) const {
        return _impl->sysErrorMessage(nativeLanguage);
    }

}