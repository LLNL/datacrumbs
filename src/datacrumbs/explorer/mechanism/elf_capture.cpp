#include <datacrumbs/explorer/mechanism/elf_capture.h>

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

std::vector<std::string> ElfSymbolExtractor::extract_symbols() {
  std::vector<std::string> symbols;
  if (!is_elf()) return symbols;

  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  const Elf64_Shdr* shdrs = reinterpret_cast<const Elf64_Shdr*>(data_ + ehdr->e_shoff);
  const char* shstrtab = reinterpret_cast<const char*>(data_ + shdrs[ehdr->e_shstrndx].sh_offset);

  for (int i = 0; i < ehdr->e_shnum; ++i) {
    if (shdrs[i].sh_type == SHT_SYMTAB || shdrs[i].sh_type == SHT_DYNSYM) {
      const Elf64_Sym* syms = reinterpret_cast<const Elf64_Sym*>(data_ + shdrs[i].sh_offset);
      size_t num_syms = shdrs[i].sh_size / shdrs[i].sh_entsize;
      const char* strtab = reinterpret_cast<const char*>(data_ + shdrs[shdrs[i].sh_link].sh_offset);

      for (size_t j = 0; j < num_syms; ++j) {
        std::string name = std::string(strtab + syms[j].st_name);
        if (!name.empty()) symbols.push_back(name);
      }
    }
  }
  return symbols;
}

bool ElfSymbolExtractor::is_elf() const {
  if (size_ < sizeof(Elf64_Ehdr)) return false;
  const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data_);
  return ehdr->e_ident[EI_MAG0] == ELFMAG0 && ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr->e_ident[EI_MAG2] == ELFMAG2 && ehdr->e_ident[EI_MAG3] == ELFMAG3;
}
}  // namespace datacrumbs