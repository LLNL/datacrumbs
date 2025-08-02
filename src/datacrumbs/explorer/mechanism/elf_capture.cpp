#include <datacrumbs/explorer/mechanism/elf_capture.h>

#include <iomanip>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace datacrumbs {
ElfSymbolExtractor::ElfSymbolExtractor(const std::string& path)
    : fd_(-1), data_(nullptr), size_(0) {
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ < 0) throw std::runtime_error("Failed to open ELF file");

  size_ = lseek(fd_, 0, SEEK_END);
  lseek(fd_, 0, SEEK_SET);

  data_ = static_cast<uint8_t*>(mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0));
  if (data_ == MAP_FAILED) throw std::runtime_error("Failed to mmap ELF file");
}

ElfSymbolExtractor::~ElfSymbolExtractor() {
  if (data_ && data_ != MAP_FAILED) munmap(data_, size_);
  if (fd_ >= 0) close(fd_);
}

std::pair<std::vector<std::string>, std::vector<std::string>>
ElfSymbolExtractor::extract_symbols() {
  auto symbols_map = std::unordered_map<std::string, std::string>();
  auto symbol_counts = std::unordered_map<std::string, int>();
  if (!is_elf()) return {std::vector<std::string>(), std::vector<std::string>()};

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
        if (ELF64_ST_TYPE(syms[j].st_info) != STT_FUNC) continue;
        std::string name = std::string(strtab + syms[j].st_name);
        if (!name.empty()) {
          symbol_counts[name]++;
        }
      }
    }
  }

  // Second pass: collect only unique symbols
  for (int i = 0; i < ehdr->e_shnum; ++i) {
    if (shdrs[i].sh_type == SHT_SYMTAB || shdrs[i].sh_type == SHT_DYNSYM) {
      const Elf64_Sym* syms = reinterpret_cast<const Elf64_Sym*>(data_ + shdrs[i].sh_offset);
      size_t num_syms = shdrs[i].sh_size / shdrs[i].sh_entsize;
      const char* strtab = reinterpret_cast<const char*>(data_ + shdrs[shdrs[i].sh_link].sh_offset);

      for (size_t j = 0; j < num_syms; ++j) {
        if (syms[j].st_shndx == SHN_UNDEF) continue;
        if (ELF64_ST_TYPE(syms[j].st_info) != STT_FUNC) continue;
        std::string name = std::string(strtab + syms[j].st_name);
        if (!name.empty() && symbol_counts[name] == 1 && symbols_map.find(name) == symbols_map.end()) {
          char buffer[32];
          sprintf(buffer, "0x%lx", static_cast<unsigned long>(syms[j].st_value));
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
  std::vector<std::string> symbols;
  std::vector<std::string> offsets;
  for (const auto& pair : symbols_map) {
    symbols.push_back(pair.first);
    offsets.push_back(pair.second);
  }
  return {symbols, offsets};
}

bool ElfSymbolExtractor::is_elf() const {
  if (size_ < sizeof(Elf64_Ehdr)) return false;
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  return ehdr->e_ident[EI_MAG0] == ELFMAG0 && ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr->e_ident[EI_MAG2] == ELFMAG2 && ehdr->e_ident[EI_MAG3] == ELFMAG3;
}
}  // namespace datacrumbs