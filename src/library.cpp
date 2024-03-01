#include "loadso/library.h"
#include "loadso/system.h"

#ifdef _WIN32
#  include <Windows.h>
// 12345
#  include <Shlwapi.h>
#  define OS_MAX_PATH MAX_PATH
#else
#  include <dlfcn.h>
#  include <limits.h>
#  include <string.h>
#  define OS_MAX_PATH PATH_MAX
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

    using DllHandlePrivate =
#ifdef _WIN32
        HMODULE
#else
        void *
#endif
        ;

    using EntryHandlePrivate =
#ifdef _WIN32
        FARPROC
#else
        void *
#endif
        ;

    class Library::Impl {
    public:
        DllHandlePrivate hDll;
        PathString path;

        Impl() {
            hDll = nullptr;
        }

        PathString getErrMsg(bool nativeLanguage) {
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

        static int nativeLoadHints(int loadHints) {
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
    };

    Library::Library() : _impl(std::make_unique<Impl>()) {
    }

    Library::~Library() {
        close();
    }

    bool Library::open(const PathString &path, int hints) {
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

        _impl->hDll = handle;
        _impl->path = path;
        return true;
    }

    bool Library::close() {
        auto handle = _impl->hDll;
        if (!handle) {
            return true;
        }

        if (!
#ifdef _WIN32
            ::FreeLibrary(handle)
#else
            (dlclose(handle) == 0)
#endif
        ) {
            return false;
        }

        _impl->hDll = nullptr;
        _impl->path.clear();
        return true;
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

    EntryHandle Library::resolve(const std::string &name) {
        auto handle = _impl->hDll;
        if (!handle) {
            return nullptr;
        }

        auto addr = (EntryHandle)
#ifdef _WIN32
            ::GetProcAddress(handle, name.data())
#else
            dlsym(handle, name.data())
#endif
            ;
        return addr;
    }

    PathString Library::lastError(bool nativeLanguage) const {
        return _impl->getErrMsg(nativeLanguage);
    }

}