
#include <datacrumbs/explorer/mechanism/header_capture.h>
namespace datacrumbs {
HeaderFunctionExtractor::HeaderFunctionExtractor(const std::string& headerPath)
    : headerPath_(headerPath), index_(nullptr), tu_(nullptr) {}

HeaderFunctionExtractor::~HeaderFunctionExtractor() {
  if (tu_) clang_disposeTranslationUnit(tu_);
  if (index_) clang_disposeIndex(index_);
}

std::vector<std::string> HeaderFunctionExtractor::extractFunctionNames() {
  std::vector<std::string> functionNames;

  index_ = clang_createIndex(0, 0);
  if (!index_) {
    std::cerr << "Failed to create Clang index." << std::endl;
    return functionNames;
  }

  tu_ = clang_parseTranslationUnit(index_, headerPath_.c_str(), nullptr, 0, nullptr, 0,
                                   CXTranslationUnit_None);

  if (!tu_) {
    std::cerr << "Failed to parse translation unit for: " << headerPath_ << std::endl;
    return functionNames;
  }

  struct VisitorData {
    std::vector<std::string>* names;
  } data{&functionNames};

  clang_visitChildren(
      clang_getTranslationUnitCursor(tu_),
      [](CXCursor cursor, CXCursor /*parent*/, CXClientData client_data) {
        auto* data = static_cast<VisitorData*>(client_data);
        if (cursor.kind == CXCursor_FunctionDecl || cursor.kind == CXCursor_CXXMethod) {
          CXString functionName = clang_getCursorSpelling(cursor);
          data->names->emplace_back(clang_getCString(functionName));
          clang_disposeString(functionName);
        }
        return CXChildVisit_Recurse;
      },
      &data);

  return functionNames;
}
}  // namespace datacrumbs