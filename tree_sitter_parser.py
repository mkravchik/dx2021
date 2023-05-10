import sys
import os
import time
import fnmatch
from tqdm import tqdm
from tree_sitter import Language, Parser


def find_functions2(node, source_code):
    functions = []

    if node.type in {'function_definition'}:  # , 'declaration'}:
        declarator = node.child_by_field_name("declarator")
        if declarator is not None:
            declarator2 = declarator.child_by_field_name("declarator")
            if declarator2 is not None:
                identifier = declarator2.child_by_field_name("name")
                # identifier = declarator.child_by_field_name("declarator").child_by_field_name("identifier")
                if identifier is not None:
                    function_name = source_code[identifier.start_byte:identifier.end_byte].decode('utf-8')
                    start_line = node.start_point[0] + 1
                    end_line = node.end_point[0] + 1
                    if node.type == 'function_definition' or \
                            (node.type == 'declaration' and node.child_by_field_name("type").has_child(
                                "function_declarator")):
                        functions.append((function_name, start_line, end_line))

    for child in node.children:
        functions += (find_functions(child, source_code))

    return functions


def find_functions(node, source_code):
    functions = []
    stack = [node]

    while stack:
        current_node = stack.pop()
        if current_node.type in {'function_definition'}:  # , 'declaration'}:
            declarator = current_node.child_by_field_name("declarator")
            if declarator is not None:
                declarator2 = declarator.child_by_field_name("declarator")
                if declarator2 is not None:
                    identifier = declarator2.child_by_field_name("name")
                    # identifier = declarator.child_by_field_name("declarator").child_by_field_name("identifier")
                    if identifier is not None:
                        function_name = source_code[identifier.start_byte:identifier.end_byte].decode('utf-8')
                        start_line = current_node.start_point[0] + 1
                        end_line = current_node.end_point[0] + 1
                        if current_node.type == 'function_definition' or \
                                (current_node.type == 'declaration' and current_node.child_by_field_name(
                                    "type").has_child("function_declarator")):
                            functions.append((function_name, start_line, end_line))

        stack.extend(current_node.children)

    return functions


def get_functions(file_path):
    if not os.path.isfile(file_path):
        print("Unsupported file. probably a link")
        return
    with open(file_path, "rb") as f:
        source_code = f.read()

    Language.build_library(
        'build/my-languages.so',
        [
            'tree-sitter-c',
            'tree-sitter-cpp',
        ]
    )

    C_LANGUAGE = Language('build/my-languages.so', 'c')
    CPP_LANGUAGE = Language('build/my-languages.so', 'cpp')

    parser = Parser()
    if file_path.endswith('.c'):
        parser.set_language(C_LANGUAGE)
    elif file_path.endswith('.cpp') or file_path.endswith('.cxx') or file_path.endswith('.cc'):
        parser.set_language(CPP_LANGUAGE)
    else:
        print("Unsupported file extension. Please use .c, .cpp, .cxx or .cc")
        return

    tree = parser.parse(source_code)

    functions = find_functions(tree.root_node, source_code)
    return functions


def count_files_in_directory(path):
    total_files = 0
    for _, _, files in os.walk(path):
        total_files += len(files)
    return total_files


def find_c_cpp_files(path):
    # Check if the given path is a valid directory
    if not os.path.isdir(path):
        print("Invalid directory path.")
        return

    # Define the file patterns for C and C++ files
    file_patterns = ['*.c', '*.cpp', '*.cxx', '*.cc']

    # Count the total number of files in the directory and subdirectories
    total_files = count_files_in_directory(path)

    # Create a progress bar
    progress_bar = tqdm(total=total_files, desc="Processing", ncols=100)

    # Walk through the directory and its subdirectories
    for root, dirs, files in os.walk(path):
        for pattern in file_patterns:
            for filename in fnmatch.filter(files, pattern):
                # if progress_bar.n >= 64808:
                # print(os.path.join(root, filename))
                if os.path.join(root, filename) == "../sources/munt-official/src/data/static_diff_data.cpp":
                    continue
                full_path = os.path.join(root, filename)
                # print(full_path)
                get_functions(full_path)
        # Update the progress bar
        progress_bar.update(len(files))

    # Close the progress bar
    progress_bar.close()


if __name__ == "__main__":
    # print(sys.getrecursionlimit())
    # sys.setrecursionlimit(100000)
    start_time = time.time()
    find_c_cpp_files("../sources")
    elapsed_time = time.time() - start_time
    print(f"The function took {elapsed_time:.2f} seconds to run.")

    # funcs = get_functions("/home/tomerg1/git/sources/ace/ace/ACE.cpp")
    # for function_name, start_line, end_line in funcs:
    #    print(f"Function: {function_name}, Start Line: {start_line}, End Line: {end_line}")

