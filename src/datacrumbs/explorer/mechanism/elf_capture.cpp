#include <datacrumbs/explorer/mechanism/elf_capture.h>

namespace datacrumbs {

ElfSymbolExtractor::ElfSymbolExtractor(const std::string& path, bool include_offsets)
    : fd_(-1), data_(nullptr), size_(0), include_offsets_(include_offsets) {
  DC_LOG_TRACE("ElfSymbolExtractor: constructor start for file: %s", path.c_str());
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ < 0) {
    DC_LOG_ERROR("Failed to open ELF file: %s", path.c_str());
    throw std::runtime_error("Failed to open ELF file");
  }

  size_ = lseek(fd_, 0, SEEK_END);
  lseek(fd_, 0, SEEK_SET);

  data_ = static_cast<uint8_t*>(mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0));
  if (data_ == MAP_FAILED) {
    DC_LOG_ERROR("Failed to mmap ELF file: %s", path.c_str());
    throw std::runtime_error("Failed to mmap ELF file");
  }
  DC_LOG_TRACE("ElfSymbolExtractor: constructor end for file: %s", path.c_str());

  if (!is_elf()) {
    DC_LOG_ERROR("File is not a valid ELF file");
    throw std::runtime_error("Not a valid ELF file");
  }
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  const Elf64_Shdr* shdrs = reinterpret_cast<const Elf64_Shdr*>(data_ + ehdr->e_shoff);
  const char* shstrtab = reinterpret_cast<const char*>(data_ + shdrs[ehdr->e_shstrndx].sh_offset);

  base_address_ = 0;
  for (int i = 0; i < ehdr->e_shnum; ++i) {
    const char* section_name = shstrtab + shdrs[i].sh_name;
    if (strcmp(section_name, ".text") == 0) {
      base_address_ = shdrs[i].sh_addr;
      DC_LOG_INFO("Found .text section at address: 0x%lx",
                  static_cast<unsigned long>(base_address_));
      break;
    }
  }
  if (base_address_ == 0) {
    DC_LOG_WARN("Could not find .text section, using e_entry as base address");
    base_address_ = ehdr->e_entry;
  }
  DC_LOG_INFO("ELF base address: 0x%lx", static_cast<unsigned long>(base_address_));
}

ElfSymbolExtractor::~ElfSymbolExtractor() {
  DC_LOG_TRACE("ElfSymbolExtractor: destructor start");
  if (data_ && data_ != MAP_FAILED) {
    munmap(data_, size_);
    DC_LOG_DEBUG("Unmapped ELF file memory");
  }
  if (fd_ >= 0) {
    close(fd_);
    DC_LOG_DEBUG("Closed ELF file descriptor");
  }
  DC_LOG_TRACE("ElfSymbolExtractor: destructor end");
}

std::vector<std::string> ElfSymbolExtractor::extract_symbols() {
  DC_LOG_TRACE("extract_symbols: start");
  auto symbols_map = std::unordered_map<std::string, std::string>();
  auto symbol_counts = std::unordered_map<std::string, int>();
  if (!is_elf()) {
    DC_LOG_ERROR("File is not a valid ELF file");
    return std::vector<std::string>();
  }

  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  const Elf64_Shdr* shdrs = reinterpret_cast<const Elf64_Shdr*>(data_ + ehdr->e_shoff);
  const char* shstrtab = reinterpret_cast<const char*>(data_ + shdrs[ehdr->e_shstrndx].sh_offset);

  // First pass: count symbol occurrences
  for (int i = 0; i < ehdr->e_shnum; ++i) {
    if (shdrs[i].sh_type == SHT_SYMTAB || shdrs[i].sh_type == SHT_DYNSYM) {
      const Elf64_Sym* syms = reinterpret_cast<const Elf64_Sym*>(data_ + shdrs[i].sh_offset);
      size_t num_syms = shdrs[i].sh_size / shdrs[i].sh_entsize;
      const char* strtab = reinterpret_cast<const char*>(data_ + shdrs[shdrs[i].sh_link].sh_offset);

      for (size_t j = 0; j < num_syms; ++j) {
        if (syms[j].st_shndx == SHN_UNDEF) continue;
        // if (ELF64_ST_BIND(syms[j].st_info) == STB_LOCAL) continue;
        if (ELF64_ST_TYPE(syms[j].st_info) != STT_FUNC) continue;

        std::string name = std::string(strtab + syms[j].st_name);
        if (!name.empty() && symbols_map.find(name) == symbols_map.end()) {
          char buffer[32];
          unsigned long offset = static_cast<unsigned long>(
              syms[j].st_value);  // - static_cast<unsigned long>(base_address_);
          DC_LOG_DEBUG("found name: %s st_value: 0x%lx offset: 0x%lx base_address: 0x%lx",
                       name.c_str(), static_cast<unsigned long>(syms[j].st_value), offset,
                       static_cast<unsigned long>(base_address_));
          sprintf(buffer, "0x%lx", offset);
          symbols_map[name] = buffer;
        }
      }
    }
  }

  // First, collect base names (before '@') for versioned symbols
  std::unordered_set<std::string> versioned_bases;
  for (const auto& pair : symbols_map) {
    const std::string& name = pair.first;
    auto at_pos = name.find('@');
    if (at_pos != std::string::npos) {
      versioned_bases.insert(name.substr(0, at_pos));
    }
  }

  // Now, remove non-versioned symbols if a versioned one with the same base exists
  for (auto it = symbols_map.begin(); it != symbols_map.end();) {
    const std::string& name = it->first;
    if (versioned_bases.count(name) > 0) {
      it = symbols_map.erase(it);
    } else {
      ++it;
    }
  }
  DC_LOG_DEBUG("Filtered non-versioned symbols if versioned exists");

  std::vector<std::string> symbols;
  for (const auto& pair : symbols_map) {
    if (include_offsets_) {
      symbols.push_back(pair.first + ":" + pair.second);
    } else {
      symbols.push_back(pair.first);
    }
  }
  DC_LOG_INFO("Extracted %zu unique function symbols", symbols.size());
  DC_LOG_TRACE("extract_symbols: end");
  return symbols;
}

bool ElfSymbolExtractor::is_elf() const {
  DC_LOG_TRACE("is_elf: start");
  if (size_ < sizeof(Elf64_Ehdr)) {
    DC_LOG_WARN("File size too small to be ELF");
    return false;
  }
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  bool result = ehdr->e_ident[EI_MAG0] == ELFMAG0 && ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
                ehdr->e_ident[EI_MAG2] == ELFMAG2 && ehdr->e_ident[EI_MAG3] == ELFMAG3;
  if (!result) {
    DC_LOG_WARN("ELF magic not found");
  }
  DC_LOG_TRACE("is_elf: end");
  return result;
}
}  // namespace datacrumbs