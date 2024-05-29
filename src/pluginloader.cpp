#include "pluginloader_p.h"
#include "pluginloader.h"

#include <vector>
#include <tuple>

#ifdef _WIN32
#  define UNICODE
#  include <Windows.h>
// 12345
#  include <Shlwapi.h>
#else
#  include <dlfcn.h>
#  include <limits.h>
#  include <string.h>
#  ifdef __APPLE__

#  else
#    include <elf.h>
#  endif

#  include <fstream>
#endif

namespace LoadSO {

#if defined(_WIN32)
    static bool readMetadataResource(HMODULE hModule, std::string *out) {
        HRSRC hResource = ::FindResourceW(hModule, L"loadso_plugin_metadata", RT_RCDATA);
        if (!hResource) {
            ::FreeLibrary(hModule);
            return false;
        }

        HGLOBAL hResourceLoad = ::LoadResource(hModule, hResource);
        if (!hResourceLoad) {
            ::FreeLibrary(hModule);
            return false;
        }

        LPVOID pData = ::LockResource(hResourceLoad);
        if (!pData) {
            ::FreeResource(hResourceLoad);
            return false;
        }

        if (IS_INTRESOURCE(pData)) {
            ::FreeResource(hResourceLoad);
            return false;
        }

        *out = std::string(static_cast<char *>(pData), ::SizeofResource(hModule, hResource));
        ::FreeResource(hResourceLoad);
        return true;
    }
#elif defined(__APPLE__)

#else
    template <class ElfHeaderType, class ProgramHeaderType, class SectionHeaderType>
    static bool readMetadataSection(std::ifstream &file, std::string *out) {
        ElfHeaderType ehdr;
        file.read(reinterpret_cast<char *>(&ehdr), sizeof(ehdr));
        if (!file.good()) {
            return false;
        }

        // Check header sizes
        if (ehdr.e_ehsize != sizeof(ElfHeaderType)) {
            return false;
        }
        if (ehdr.e_phentsize != sizeof(ProgramHeaderType)) {
            return false;
        }
        if (ehdr.e_shentsize != sizeof(SectionHeaderType) && ehdr.e_shentsize != 0) {
            return false;
        }

        // Read section headers
        file.seekg(ehdr.e_shoff, std::ios::beg);
        std::vector<SectionHeaderType> shdrs(ehdr.e_shnum);
        file.read(reinterpret_cast<char *>(shdrs.data()), ehdr.e_shnum * ehdr.e_shentsize);
        if (!file.good()) {
            return false;
        }

        // Read section header string table
        if (ehdr.e_shstrndx >= ehdr.e_shnum) {
            return false;
        }
        auto &shstrtab_hdr = shdrs[ehdr.e_shstrndx];
        std::vector<char> shstrtab(shstrtab_hdr.sh_size);
        file.seekg(shstrtab_hdr.sh_offset, std::ios::beg);
        file.read(shstrtab.data(), shstrtab.size());
        if (!file.good()) {
            return false;
        }

        // Find metadata section
        for (const auto &shdr : shdrs) {
            if (strcmp(&shstrtab[shdr.sh_name], ".loadso_plugin_metadata") == 0) {
                std::string section_data;
                section_data.resize(shdr.sh_size);
                file.seekg(shdr.sh_offset, std::ios::beg);
                file.read(&section_data[0], section_data.size());
                if (!file.good()) {
                    return false;
                }
                std::swap(*out, section_data);
                return true;
            }
        }
        return false;
    }

    static bool readMetadataFromELF(std::ifstream &file, std::string *out) {
        unsigned char e_ident[EI_NIDENT];
        file.read(reinterpret_cast<char *>(e_ident), sizeof(e_ident));
        if (!file.good()) {
            return false;
        }

        // Check header: ELF
        if (memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
            return false;
        }

        // Check bits
        if (e_ident[EI_CLASS] == ELFCLASS64) {
#  if __WORDSIZE == 32
            return false;
#  else
            file.seekg(0, std::ios::beg);
            return readMetadataSection<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(file, out);
#  endif
        } else if (e_ident[EI_CLASS] == ELFCLASS32) {
#  if __WORDSIZE != 32
            file.seekg(0, std::ios::beg);
            return readMetadataSection<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(file, out);
#  else
            return false;
#  endif
        }

        // Unknown type
        return false;
    }
#endif

    void PluginLoader::Impl::getMetaData() const {
        if (path.empty())
            return;

#ifdef _WIN32
        // Windows: Parse PE Resource
        HMODULE hModule = ::LoadLibraryExW(path.data(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
        if (!hModule) {
            return;
        }
        std::ignore = readMetadataResource(hModule, &metaData);
        ::FreeLibrary(hModule);
#elif defined(__APPLE__)
            // Mac: Parse Mach-O Section
#else
        // Linux: Parse ELF Section
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return;
        }
        std::ignore = readMetadataFromELF(file, &metaData);
#endif
    }

    PluginLoader::PluginLoader(const PathString &path) : _impl(std::make_unique<Impl>()) {
        _impl->path = path;
    }

    PluginLoader::~PluginLoader() = default;

    PluginLoader::PluginLoader(PluginLoader &&other) noexcept {
        std::swap(_impl, other._impl);
    }

    PluginLoader &PluginLoader::operator=(PluginLoader &&other) noexcept {
        if (this == &other)
            return *this;
        std::swap(_impl, other._impl);
        return *this;
    }

    void *PluginLoader::instance() const {
        return _impl->pluginInstance;
    }

    const std::string &PluginLoader::metaData() const {
        if (!_impl->metaDataLoaded) {
            _impl->getMetaData();
            _impl->metaDataLoaded = true;
        }
        return _impl->metaData;
    }

    bool PluginLoader::load(int hints) {
        if (!_impl->open(hints)) {
            return false;
        }

        using InstanceEntry = void *(*) ();

        auto instance_entry =
            reinterpret_cast<InstanceEntry>(_impl->resolve("loadso_plugin_instance"));
        if (!instance_entry) {
            std::ignore = _impl->close();
            return false;
        }
        _impl->pluginInstance = instance_entry();
        return true;
    }

    bool PluginLoader::unload() {
        return _impl->close();
    }

    bool PluginLoader::isLoaded() const {
        return _impl->hDll != nullptr;
    }

    PathString PluginLoader::fileName() const {
        return _impl->path;
    }

    void PluginLoader::setFileName(const PathString &fileName) {
        if (_impl->path == fileName)
            return;

        if (_impl->hDll) {
            _impl->close();
        }
        _impl->metaData.clear();
        _impl->metaDataLoaded = false;
        _impl->pluginInstance = nullptr;
        _impl->path = fileName;
    }

    PathString PluginLoader::lastError(bool nativeLanguage) const {
        return _impl->sysErrorMessage(nativeLanguage);
    }

}