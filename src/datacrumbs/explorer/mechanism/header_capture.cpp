
#include <datacrumbs/explorer/mechanism/header_capture.h>
namespace datacrumbs {

// Constructor: Initializes the extractor with the given header file path.
HeaderFunctionExtractor::HeaderFunctionExtractor(const std::string& headerPath)
    : headerPath_(headerPath), index_(nullptr), tu_(nullptr) {}

// Destructor: Cleans up Clang resources.
HeaderFunctionExtractor::~HeaderFunctionExtractor() {
  if (tu_) clang_disposeTranslationUnit(tu_);
  if (index_) clang_disposeIndex(index_);
}

// Extracts all function and method names from the header file.
std::vector<std::string> HeaderFunctionExtractor::extractFunctionNames() {
  DC_LOG_TRACE("HeaderFunctionExtractor::extractFunctionNames - start");

  std::vector<std::string> functionNames;

  // Create a Clang index for parsing.
  index_ = clang_createIndex(0, 0);
  if (!index_) {
    DC_LOG_ERROR("Failed to create Clang index.");
    return functionNames;
  }

  // Parse the translation unit (header file).
  tu_ = clang_parseTranslationUnit(index_, headerPath_.c_str(), nullptr, 0, nullptr, 0,
                                   CXTranslationUnit_None);

  if (!tu_) {
    DC_LOG_ERROR("Failed to parse translation unit for: %s", headerPath_.c_str());
    return functionNames;
  }

  // Visitor data structure to collect function names.
  struct VisitorData {
    std::vector<std::string>* names;
  } data{&functionNames};

  // Visit all children in the translation unit to find function and method declarations.
  clang_visitChildren(
      clang_getTranslationUnitCursor(tu_),
      [](CXCursor cursor, CXCursor /*parent*/, CXClientData client_data) {
        auto* data = static_cast<VisitorData*>(client_data);
        // Check if the cursor is a function or C++ method declaration.
        if (cursor.kind == CXCursor_FunctionDecl || cursor.kind == CXCursor_CXXMethod) {
          CXString functionName = clang_getCursorSpelling(cursor);
          data->names->emplace_back(clang_getCString(functionName));
          clang_disposeString(functionName);
        }
        return CXChildVisit_Recurse;
      },
      &data);

  DC_LOG_TRACE("HeaderFunctionExtractor::extractFunctionNames - end");
  return functionNames;
}

}  // namespace datacrumbs