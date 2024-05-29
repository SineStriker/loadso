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
#    include <mach-o/loader.h>
#    include <mach-o/fat.h>
#  else
#    include <elf.h>
#  endif

#  include <fstream>
#endif

#define LOADSO_PLUGIN_IDENTIFIER "loadso_metadata"

namespace LoadSO {

#if defined(_WIN32)
    static bool readMetadataResource(HMODULE hModule, std::string *out) {
        HRSRC hResource = ::FindResourceW(hModule, LOADSO_STR(LOADSO_PLUGIN_IDENTIFIER), RT_RCDATA);
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
    static bool readSectionData(std::ifstream &file, uint32_t offset, uint32_t size,
                                std::string *out) {
        std::string buffer;
        buffer.resize(size);
        file.seekg(offset, std::ios::beg);
        file.read(&buffer[0], size);
        if (!file.good()) {
            return false;
        }
        std::swap(*out, buffer);
        return true;
    }

    static bool readMetadataFromMachO(std::ifstream &file, std::string *out) {
        // Read head
        mach_header header;
        file.read(reinterpret_cast<char *>(&header), sizeof(header));

        // Check bits
        uint32_t ncmds;
        bool is64bit = false;
        if (header.magic == MH_MAGIC) {
#  if __WORDSIZE == 32
            ncmds = header.ncmds;
#  else
            return false;
#  endif
        } else if (header.magic == MH_MAGIC_64) {
#  if __WORDSIZE == 64
            mach_header_64 header64;
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(&header64), sizeof(header64));
            ncmds = header64.ncmds;
            is64bit = true;
#  else
            return false;
#  endif
        } else {
            return false;
        }

        // Traverse load commands
        for (uint32_t i = 0; i < ncmds; ++i) {
            load_command lc;
            file.read(reinterpret_cast<char *>(&lc), sizeof(lc));

            if (lc.cmd == LC_SEGMENT) {
                segment_command seg;
                file.seekg(-static_cast<int>(sizeof(load_command)), std::ios::cur);
                file.read(reinterpret_cast<char *>(&seg), sizeof(seg));

                if (std::strcmp(seg.segname, "__TEXT") == 0) {
                    for (uint32_t j = 0; j < seg.nsects; ++j) {
                        section sec;
                        file.read(reinterpret_cast<char *>(&sec), sizeof(sec));
                        if (std::strcmp(sec.sectname, LOADSO_PLUGIN_IDENTIFIER) == 0) {
                            return readSectionData(file, sec.offset, sec.size, out);
                        }
                    }
                } else {
                    file.seekg(seg.cmdsize - sizeof(segment_command), std::ios::cur);
                }
            } else if (lc.cmd == LC_SEGMENT_64) {
                // 64 bit segment
                segment_command_64 seg64;
                file.seekg(-static_cast<int>(sizeof(load_command)), std::ios::cur);
                file.read(reinterpret_cast<char *>(&seg64), sizeof(seg64));
                if (std::strcmp(seg64.segname, "__TEXT") == 0) {
                    for (uint32_t j = 0; j < seg64.nsects; ++j) {
                        section_64 sec64;
                        file.read(reinterpret_cast<char *>(&sec64), sizeof(sec64));
                        if (std::strcmp(sec64.sectname, LOADSO_PLUGIN_IDENTIFIER) == 0) {
                            return readSectionData(file, sec64.offset, sec64.size, out);
                        }
                    }
                } else {
                    file.seekg(seg64.cmdsize - sizeof(segment_command_64), std::ios::cur);
                }
            } else {
                file.seekg(lc.cmdsize - sizeof(load_command), std::ios::cur);
            }
        }
        return false;
    }
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
            if (strcmp(&shstrtab[shdr.sh_name], "." LOADSO_PLUGIN_IDENTIFIER) == 0) {
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
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return;
        }
        std::ignore = readMetadataFromMachO(file, &metaData);
#else
        // Linux: Parse ELF Section
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return;
        }
        std::ignore = readMetadataFromELF(file, &metaData);
#endif
    }

    PluginLoader::PluginLoader(const PathString &path) : _impl(new Impl()) {
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

    PathString PluginLoader::path() const {
        return _impl->path;
    }

    void PluginLoader::setPath(const PathString &fileName) {
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