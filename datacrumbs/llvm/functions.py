import clang.cindex
import sys
import re

class Functions:
    def __init__(self, header_file, regex=None):
        self.header_file = header_file
        self.regex = regex

    def get_function_names(self):
        # Read the header file
        index = clang.cindex.Index.create()
        tu = index.parse(self.header_file, args=['-D__KERNEL__'],)
        functions = []
        def visit(node):
            # print(f"Visiting node: {node.spelling} ({node.kind})")
            if node.kind == clang.cindex.CursorKind.FUNCTION_DECL:
                if node.is_definition() or node.location.file and node.location.file.name == self.header_file:
                    func_name = node.spelling
                    if self.regex is not None:
                        if not re.match(self.regex, func_name):
                            return
                functions.append(func_name)
            for child in node.get_children():
                visit(child)
        visit(tu.cursor)
        return functions
    
if __name__ == "__main__":

    regex = None
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <header_file> <regex>")
        sys.exit(1)
    if len(sys.argv) == 3:
        regex = sys.argv[2]
    header_file = sys.argv[1]
    functions = Functions(header_file, regex)
    function_names = functions.get_function_names()

    print("Function names found:")
    for name in function_names:
        print(name)