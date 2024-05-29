#ifndef LIBRARY_P_H
#define LIBRARY_P_H

#include <loadso/library.h>

namespace LoadSO {

    class Library::Impl {
    public:
        void *hDll = nullptr;
        PathString path;

        virtual ~Impl();

        static int nativeLoadHints(int loadHints);
        static PathString sysErrorMessage(bool nativeLanguage);

        bool open(int hints = 0);
        bool close();
        void *resolve(const char *name) const;
    };

}

#endif // LIBRARY_P_H
