import sys
import clang
import clang.cindex
import json
import os
import shutil
import argparse
from tqdm import tqdm
from ClassMap.classMap import mapper
import re
import platform
from tree_sitter import Language, Parser

USE_TREE_SITTER = False
if USE_TREE_SITTER:
    # tree-sitter initialization
    Language.build_library( # Store the library in the `build` directory
        'build/my-languages.so',
        # Include one or more languages
        # Assumes the parsers are in the `../tree-sitter` directory
        # Get them using `git clone https://github.com/tree-sitter/tree-sitter-XXX
        [
            '../tree-sitter/tree-sitter-c',
            '../tree-sitter/tree-sitter-cpp',
        ]
    )

    C_LANGUAGE = Language('build/my-languages.so', 'c')
    CPP_LANGUAGE = Language('build/my-languages.so', 'cpp')

DEBUG = False

combined_jsonl = "all.jsonl"
train_jsonl = "train.jsonl"
test_jsonl = "test.jsonl"
valid_jsonl = "valid.jsonl"
folder_path = "/mnt/d/GitHub_Clones/scripts/C_Dataset"
train_ratio = 0.8
test_ratio = 0.1
max_lines = 0

this_os = platform.system().lower()
if this_os == 'linux':
    clang_path = "/usr/lib/llvm-10/lib/libclang.so.1"
elif this_os == 'windows':
    clang_path = "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/libclang.dll"
else:
    print("Unknown OS:", this_os, ". Don't know where clang is");
    exit(1)

clang.cindex.Config.set_library_file(clang_path)
# the API is best described at https://opensource.apple.com/source/lldb/lldb-112/llvm/tools/clang/bindings/python/clang/cindex.py.auto.html
# Parse options: (not all of them are present in the Python file)
CXTranslationUnit_None = 0x0
CXTranslationUnit_DetailedPreprocessingRecord = 0x01
CXTranslationUnit_Incomplete = 0x02
CXTranslationUnit_PrecompiledPreamble = 0x04 
CXTranslationUnit_CacheCompletionResults = 0x08
CXTranslationUnit_ForSerialization = 0x10
CXTranslationUnit_CXXChainedPCH = 0x20
CXTranslationUnit_SkipFunctionBodies = 0x40
CXTranslationUnit_IncludeBriefCommentsInCodeCompletion = 0x80
CXTranslationUnit_CreatePreambleOnFirstParse = 0x100
CXTranslationUnit_KeepGoing = 0x200
CXTranslationUnit_SingleFileParse = 0x400
CXTranslationUnit_LimitSkipFunctionBodiesToPreamble = 0x800
CXTranslationUnit_IncludeAttributedTypes = 0x1000
CXTranslationUnit_VisitImplicitAttributes = 0x2000
CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles = 0x4000
CXTranslationUnit_RetainExcludedConditionalBlocks = 0x8000

def method_definitions(cursor):
    for i in cursor.walk_preorder():
        # print(i.kind, i.location.file.name if i.location.file else "", i.extent.start.line, i.extent.end.line)
        if i.kind != clang.cindex.CursorKind.CXX_METHOD and i.kind != clang.cindex.CursorKind.FUNCTION_DECL:
            continue
        if not i.is_definition():
            continue
        yield i

def srcrangestr(x):
    return '%s:%d:%d - %s:%d:%d' % (x.start.file, x.start.line, x.start.column, x.end.file, x.end.line, x.end.column)


def get_ifdefs(file_path: str) -> list:
    """This function extracts the ifdefs from the file
    Clang does not have any ifdefs defined thus it skips everything under ifdef
    Alternatively, we could remove the ifdefs, but for that we need to use the parser 
    that will find the matching #else/#endif and I did not find a way to do it yet.
    Note: this is a very primitive mechanism as it does not work recursively on the file's includes
    Args:
        file_path (_type_): _description_
    """    
    # index = clang.cindex.Index.create()
    # tu = index.parse(file_path, args=['-x', 'c++'])
    # for x in tu.cursor.get_tokens():
    #     print (x.kind)
    #     print ("  " + srcrangestr(x.extent))
    #     print ("  '" + str(x.spelling) + "'")
    res = []
    try:
        with open(file_path) as src:
            for line in src.readlines(): 
                # taking care of the simple cases
                # single line, no conditions
                # A proper treatment requires using a proper preprocessor
                match = re.findall("#if defined\(([A-Za-z0-9_-]+)\)$", line)
                if len(match) == 0:
                    match = re.findall("#ifdef\(([A-Za-z0-9_-]+)\)$", line)
                if len(match) == 0:
                    match = re.findall("#ifdef\s+([A-Za-z0-9_-]+)\s*$", line)
                if len(match):
                    res.append(match[0])
    except Exception as e:
        print(f"Failed parsing {file_path}, error {e}")
        return res

    return res

def visit(node: clang.cindex.Cursor):
    print(node.displayname + ' ' + str(node.kind) + ' ' + str(node.location))
    for child in node.get_children():
        visit(child)

def dump_functions(file_path, project, out_file_path, max_lines = max_lines, min_lines = 1,
    label = None, include_dirs = None, dest_set = None, defines = []):
    # print("dump_functions", file_path, project)
    # necessary to deal with the symlinks
    file_path = os.path.realpath(file_path)

    # some libraries put lots of code under if defined - include this code
    args = []
    for define in defines:
        args.extend(["-D", define])
    expected_defines = set(get_ifdefs(file_path))
    for define in expected_defines:
        args.extend(["-D", define])

    # print(args)

    if include_dirs is not None:
        for inc in include_dirs:
            args.extend(["-I", inc])

    # print(args)

    index = clang.cindex.Index.create()
    try:
        tu = index.parse(file_path, args=args,
        # is this required?
          options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
        )
        if DEBUG and len(tu.diagnostics):
            print(list(tu.diagnostics)) 
    except Exception as e:
        print(f"Failed parsing {file_path}, error {e}")
        return

    defns = method_definitions(tu.cursor)

    with open(file_path, encoding = "ISO-8859-1") as src:
        lines = src.readlines()

    file_name = file_path.split(os.sep)[-1]
    
    with open(out_file_path, "at") as jsonl:
        funcs_found = 0  
        for function_node in defns:
            # print("DEF: ", function_node.location.file, function_node.displayname)
            if function_node.location.file.name != file_path:
                continue
            if function_node.extent.end.line - function_node.extent.start.line > min_lines:
                funcs_found += 1
                # print(function_node.location.file, function_node.displayname)
                # TODO - consider columns as well
                func_lines = lines[function_node.extent.start.line - 1 : function_node.extent.end.line]
                if max_lines > 0:
                   func_lines = func_lines[:max_lines] 
                elif max_lines < 0:
                   func_lines = func_lines[max_lines:] 
                body = "".join(func_lines)
                #body = body.encode("unicode_escape").decode("utf-8")
                # print(body)
                # for line in range(function_node.extent.start.line, function_node.extent.end.line + 1):
                #     print(lines[line - 1], end='')
                # The replace below is required to get rid of the double backquotes introduced by json's dump and dumps
                # func_dict = {"project": project, "file":file_name, "func": body, "func_name": function_node.spelling}
                func_dict = {"project": project, "file":file_name, "func": body,
                 "start_line": function_node.extent.start.line, "end_line": function_node.extent.end.line,
                  "func_name": function_node.spelling}
                if label is not None:
                    func_dict['label'] = label
                if dest_set is not None:
                    func_dict['set'] = dest_set

                json_s = json.dumps(func_dict)#.replace("\\\\","\\")

                jsonl.write(json_s)
                jsonl.write("\n")
        #if funcs_found == 0:
        if DEBUG:
            print(f"{funcs_found} functions found in {file_path}")


def walkdir(folder):
    """Walk through every file in a directory"""
    for root, dirs, files in os.walk(folder):
        for filename in files:
            yield root, filename


def parse_sources(location, out_file_path=combined_jsonl, max_lines=max_lines, class_map=None, set_map=False, dump_all=False):
    
    # delete the existing out file
    if os.path.exists(out_file_path):
        os.remove(out_file_path)
    
    # Precomputing files count
    files_count = 0
    for _ in tqdm(walkdir(location)):
        files_count += 1

    class_mapper = None
    label = None
    inc_dirs = None
    if class_map is not None:
        class_mapper = mapper(class_map, location)

    print(f"Found {files_count} files in {location}")
    # Computing for real
    for root, filename in tqdm(walkdir(location), total=files_count):
        if filename.endswith(".cpp") or filename.endswith(".c"):
            dump = True
            project = root[len(location) + len(os.sep):].split(os.sep)[0]
            if class_mapper is not None:
                # add only mapped files
                label, inc_dirs, project, defines = class_mapper.getFileClass(os.path.sep.join([root, filename]))
                if label.lower() == "unknown" and not dump_all:
                    dump = False
                if set_map:
                    dest_set = class_mapper.getProjectSet(project)
                else:
                    dest_set = None
            if dump:
                dump_functions(os.path.join(root, filename), project, out_file_path,
                 max_lines, label=label, include_dirs=inc_dirs, dest_set=dest_set, defines=defines)


def split_labeled_dataset(combined_jsonl_path, val_ratio, label, class_map=None):
    """
    This function opens the combined file and writes each line into its designated set
    If no validation lines were found, the train set is split into train and validation
    according to the given ratio
    """
    print("split_labeled_dataset", locals())
    class_mapper = None
    if class_map is not None:
        class_mapper = mapper(class_map)

    # to save memory, first read though the file looking for the validation set
    validation_present = False
    with open(combined_jsonl_path) as src:
        for line in src:
            if len(line):
                func = json.loads(line)
                dest_set = func.get('set', None)
                if dest_set is None: # can happen in _lines files
                    if class_mapper:
                        dest_set = class_mapper.getProjectSet(func.get('project', None))
                if dest_set == 'validation':
                    validation_present = True
                    print("Validation set found")
                    break

    train_f = open(train_jsonl, "wt")
    test_f = open(test_jsonl, "wt")
    valid_f = open(valid_jsonl, "wt")

    if validation_present:
        with open(combined_jsonl_path) as src:
            train_cnt = valid_cnt = test_cnt = 0
            for line in src:
                try:
                    if len(line):
                        func = json.loads(line)
                        dest_set = func.get('set', None)
                        if dest_set is None: # can happen in _lines files
                            if class_mapper:
                                dest_set = class_mapper.getProjectSet(func.get('project', None))

                        if dest_set is not None:
                            clean_line = json.dumps(func)+"\n"
                            if dest_set == 'train':
                                train_f.write(clean_line)
                                train_cnt += 1
                            elif dest_set == 'validation':
                                valid_f.write(clean_line)
                                valid_cnt += 1
                            elif dest_set == 'test':
                                test_f.write(clean_line)
                                test_cnt += 1
                            else:
                                print("Unknown destination: " + line)
                except Exception as e:
                    print("Skipping invalid line", line[:160])
        print(f"train {train_cnt}, val {valid_cnt}, test {test_cnt} ")
        return

    with open(combined_jsonl_path) as src:
        lines = src.readlines()

    curr_proj = ""
    curr_label = ""   
    train_lines = []
    valid_lines = []

    def _write_splits():
        nonlocal train_lines

        train_end = len(train_lines) - int(len(train_lines)*val_ratio)
        for idx in range(train_end):
            train_f.write(train_lines[idx])
        for idx in range(train_end, len(train_lines)):
            valid_f.write(train_lines[idx])
        print("%s %s train %d, val %d" % (curr_proj, curr_label, train_end, len(train_lines) - train_end))
        train_lines = []

    for l_idx, line in enumerate(lines):
        try:
            if len(line):
                func = json.loads(line)
                if func['project'] != curr_proj or func[label] != curr_label:
                    if curr_proj != "" and curr_label != "":
                        # we completed the previous project
                        _write_splits()
                curr_proj = func['project']
                curr_label = func[label]
                dest_set = func.get('set', None)
                if dest_set is not None:
                    del func['set']
                clean_line = json.dumps(func)+"\n"
                if dest_set == 'train':
                    train_lines.append(clean_line)
                if dest_set == 'train':
                    train_lines.append(clean_line)
                else:
                    test_f.write(clean_line)
        except Exception as e:
            print("Skipping invalid line", line[:160])
            lines[l_idx] = "" # This will mark it as invalid

    _write_splits()

def split_dataset(combined_jsonl_path, train_ratio, test_ratio, use_defined_set=False, write_lines=False, label='label', class_map=None):
    # we need to read the lines counting them for each project, then split
    # assumptions:
    # 1. Continuity of the projects in the combined file
    # 2. The entire combined files fit into memory

    if use_defined_set:
        split_labeled_dataset(combined_jsonl_path, 1.0 - (train_ratio + test_ratio), label, class_map)
        return

    with open(combined_jsonl_path) as src:
        lines = src.readlines()

    train_f = open(train_jsonl, "wt")
    test_f = open(test_jsonl, "wt")
    valid_f = open(valid_jsonl, "wt")

    if write_lines:
        train_lines_f = open("train_line_nums", "wt")
        valid_lines_f = open("valid_line_nums", "wt")


    curr_proj = ""
    curr_label = ""   
    start = -1
    end = -1
    
    
    def _write_splits():
        train_end = int(start + (end - start)*train_ratio)
        for idx in range(start, train_end):
            if len(lines[idx]):
                train_f.write(lines[idx])
                if write_lines:
                    train_lines_f.write(str(idx+1)+"\n")

        val_end = train_end + int((end - start)*(1.0 - (train_ratio + test_ratio)))
        for idx in range(train_end, val_end):
            if len(lines[idx]):
                valid_f.write(lines[idx])
                if write_lines:
                    valid_lines_f.write(str(idx+1)+"\n")
        for idx in range(val_end, end):
            if len(lines[idx]):
                test_f.write(lines[idx])
        print("%s %s: from %d to %d. train_end %d, val_end %d" % (curr_proj, curr_label, start, end, train_end, val_end))
    for l_idx, line in enumerate(lines):
        try:
            func = json.loads(line)
            if func['project'] != curr_proj or func[label] != curr_label:
                end = l_idx
                if start != -1:
                    # we completed the previous project or label
                    _write_splits()
                curr_proj = func['project']
                curr_label = func[label]
                start = end
        except Exception as e:
            print("Skipping invalid line", line[:160])
            lines[l_idx] = "" # This will mark it as invalid
    end = l_idx
    _write_splits()


def get_file_functions_clang(file_path, include_dirs = None, defines = [], snippet=None):
    """
    Returns a list of functions in the file. 
    For each function, returns a tuple of (displayname, start_line, end_line)
    """
    file_path = os.path.realpath(file_path)

    # some libraries put lots of code under if defined - include this code
    args = []
    for define in defines:
        args.extend(["-D", define])
    expected_defines = set(get_ifdefs(file_path))
    for define in expected_defines:
        args.extend(["-D", define])

    # print(args)

    if include_dirs is not None:
        for inc in include_dirs:
            args.extend(["-I", inc])

    # print(args)

    try:
        index = clang.cindex.Index.create()
        # We do not need the functions, just their limits
        tu = index.parse(file_path, args=args, 
                         options = CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_LimitSkipFunctionBodiesToPreamble)
        functions = []
        for node in tu.cursor.walk_preorder():
            if node.location.file is not None and node.location.file.name == file_path:
                if (node.kind == clang.cindex.CursorKind.CXX_METHOD or \
                    node.kind == clang.cindex.CursorKind.FUNCTION_DECL) and \
                        node.is_definition():
                    functions.append((node.displayname, node.extent.start.line, node.extent.end.line))
        if len(functions) == 0:
            print("No functions in file: %s" % file_path)
        return functions
    except Exception as e:
        print("Skipping file %s due to error %s" % (file_path, str(e)))
        return []


if USE_TREE_SITTER:
    def get_file_functions_ts(file_path, include_dirs = None, defines = [], snippet=None):
        """
        Returns a list of functions in the file using tree_sitter 
        For each function, returns a tuple of (displayname, start_line, end_line)
        """
        file_path = os.path.realpath(file_path)

        try:
            parser = Parser()
            if file_path.endswith('.c'):
                parser.set_language(C_LANGUAGE)
            elif file_path.endswith('.cpp') or file_path.endswith('.cxx') or file_path.endswith('.cc'):
                parser.set_language(CPP_LANGUAGE)
            else:
                print("Unsupported file extension. Please use .c, .cpp, .cxx or .cc")
                return  []

            with open(file_path, "rb") as src:
                tree = parser.parse(src.read())

            functions = []
            stack = [tree.root_node]
            while stack:
                node = stack.pop()
                if node.type == "function_definition":
                    start_line = node.start_point[0] + 1 # they are zero-based
                    end_line = node.end_point[0] + 1 # they are zero-based
                    displayname_parts = [str(child.text, "utf-8") for child in node.children if child.type != "compound_statement"] #node.children[1].children[0].text
                    displayname = " ".join(displayname_parts)
                    functions.append((displayname, start_line, end_line))
                stack.extend(node.children)

            if len(functions) == 0:
                print("No functions in file: %s" % file_path)
            return functions
        except Exception as e:
            print("Skipping file %s due to error %s" % (file_path, str(e)))
            return []

def get_file_functions(file_path, include_dirs = None, defines = [], snippet=None):
    if USE_TREE_SITTER:
        return get_file_functions_ts(file_path, include_dirs, defines, snippet)
    else:
        return get_file_functions_clang(file_path, include_dirs, defines, snippet)


def find_function(file_path, start_line, end_line, include_dirs = None, defines = [], snippet=None):
    # print("dump_functions", file_path, project)
    # necessary to deal with the symlinks
    file_path = os.path.realpath(file_path)

    file_functions = get_file_functions(file_path, include_dirs, defines, snippet)

    with open(file_path, encoding = "ISO-8859-1") as src:
        lines = src.readlines()

    file_name = file_path.split(os.sep)[-1]

    for function in file_functions:
        # print("DEF: ", function[0], function[1], function[2])
        if function[1] <= start_line and function[2]>= end_line:
            if DEBUG:
                print(f"Found: {function[0]}")
            func_lines = lines[function[1] - 1 : function[2]]
            body = "".join(func_lines)
            # Verifying the snippet is indeed there
            supposed_snippet = "".join("".join(func_lines[start_line - function[1] :
                                                   end_line - function[1] + 1]).split())
            if supposed_snippet != "".join(snippet.split()):
                print (f"Did not find {snippet[:20]}... in {file_path} between {start_line} and {end_line}")
                return "", 0, 0 
            return body, start_line - function[1], end_line - function[1] + 1
        elif (function[1] >= start_line and \
              function[2] >= end_line and\
                function[1] <= end_line ) or \
            (function[1] <= start_line and \
             function[2] <= end_line and \
                function[2] >= start_line ):
            print(f"Snippet spreads beyond a function. {start_line}, {end_line},\
                   {file_path}, {function[0]}, {function[1]} - {function[2]}")
            
    print (f"Did not find {snippet[:20]}... in {file_path} between {start_line} and {end_line}")
    return "", 0, 0


"""
Converts the file_path field of each item in the combined_json_path file to an absolute local path pointed by the location parameter.
Converting the slashes to the local OS
"""
def local_path(orig_source_location, location, combined_jsonl_path):
    tmp_name = combined_jsonl_path + ".tmp"
    updated_jsonl = open(tmp_name, "wt")
    with open(combined_jsonl_path) as src:
        for line in src:
            item = json.loads(line)
            item["file_path"] = item["file_path"].replace(orig_source_location, location)
            item["file_path"] = os.path.abspath(item["file_path"].replace('\\' , os.sep))
            updated_jsonl.write(json.dumps(item) + "\n")
    updated_jsonl.close()
    os.rename(tmp_name, combined_jsonl_path)

"""
Adds a full function body to each jsonl line base on the file_name, start_line, end_line. 
The file name should be relative to the *location* argument
You can use something like sed 's/C:\\\\Users\\\\Or\\\\Desktop\\\\Fatal-Library\\\\sources\\\\//g' -i <JSONL_PATH> to get the path right
"""
def add_function_body(location, combined_jsonl_path, class_map):
    class_mapper = mapper(class_map, location)

    # Open the input jsonl
    # Open a tmp output (?)
    tmp_name = combined_jsonl_path + ".tmp"
    updated_jsonl = open(tmp_name, "wt")
    # For each line
    # Find the source file
    # Parse it
    # Find the relevant function
    # Write the updated line, from, to

    lines_count = 0
    with open(combined_jsonl_path) as src:
        for line in src:
            lines_count += 1
    with open(combined_jsonl_path) as src:
        for line in tqdm(src, total=lines_count):
            if len(line):
                try:
                    func = json.loads(line)
                except Exception as e:
                    print(f"Failed parsing {line}, error {e}")
                    continue
                if 'full_func' in func:
                        print("The file already contains full function body, skipping")
                        return

                if '\\' in func["file_path"] and '\\' != os.sep:
                    func["file_path"] = os.sep.join(func["file_path"].split('\\'))
                #remove specific path
                if os.path.exists(func["file_path"]):
                    norm_file_path = func["file_path"]
                else:
                    if func["file_path"].find("sources") != -1:
                        func["file_path"] = os.sep.join(func["file_path"].split(os.sep)[func["file_path"].split(os.sep).index('sources')+1:])
                        norm_file_path = os.path.abspath(location + os.sep + func["file_path"])
                    else:
                        print("Don't know how to normalize " + func["file_path"])
                        continue

                if os.path.exists(norm_file_path):
                    if DEBUG:
                        print(f'Looking for a function owning a snippet between {func["start_line"]} and {func["end_line"]} in {norm_file_path}')

                    if class_mapper is not None:
                        # add only mapped files
                        label, inc_dirs, project, defines = class_mapper.getFileClass(norm_file_path)
                        if label.lower() == "unknown":
                            print(f"{norm_file_path} not found in the map, skipping")
                            continue
                        full_func, begin, end = find_function(norm_file_path, func["start_line"], func["end_line"],
                                                               inc_dirs, defines, func["func"])
                        if len(full_func):
                            func["full_func"] = full_func
                            func["begin"] = begin
                            func["end"] = end

                            json_s = json.dumps(func)

                            updated_jsonl.write(json_s)
                            updated_jsonl.write("\n")
                else:
                    print(f"{norm_file_path} not found")

    updated_jsonl.close()
    shutil.move(tmp_name, combined_jsonl_path)
    pass

# dump_functions(
#     #"/mnt/d/GitHub_Clones/scripts/C_Dataset/test/check_datasets/UI/7zip/GUI/BenchmarkDialog.cpp",
#     #"/mnt/d/GitHub_Clones/scripts/C_Dataset/test/check_datasets/Net/poco/Net/src/HTTPHeaderStream.cpp",
#     # "/mnt/d/GitHub_Clones/scripts/C_Dataset/mbedtls/library/aes.c",
#     "/mnt/d/GitHub_Clones/scripts/C_Dataset/test/poco/Net/src/HTTPRequestHandler.cpp",
#     "mbedtls", "/tmp/a.jsonl",
#     include_dirs = ["/mnt/d/GitHub_Clones/scripts/C_Dataset/test/poco/Crypto/include",
#         "/mnt/d/GitHub_Clones/scripts/C_Dataset/test/poco/Foundation/include",
#         "/mnt/d/GitHub_Clones/scripts/C_Dataset/test/poco/Net/include"]
#     )
# exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--location", help="C/C++ files location. Defaults to %s." % folder_path, default=folder_path)
    parser.add_argument("-jl", "--jsonl_location", help="The combined JSONL file location. Defaults to %s." % combined_jsonl, default=combined_jsonl)
    parser.add_argument("-s", "--split", help="Perform splitting to train/validation/test. Defaults to false.", action='store_true')
    parser.add_argument("-np", "--no_parse", help="Do not parse the sources. Defaults to false.", action='store_true')
    parser.add_argument("-train", "--train_ratio", type=float, help="The ratio of the data to out into train.jsonl. Defaults to %.1f." % train_ratio, default=train_ratio)
    parser.add_argument("-test", "--test_ratio", type=float, help="The ratio of the data to out into test.jsonl. Defaults to %.1f." % test_ratio, default=test_ratio)
    parser.add_argument("-maxl", "--max_lines", type=int, help="The number of function lines to keep. 0 means - use the entire function. Negative values keep the last lines. Defaults to %d." % max_lines, default=max_lines)
    parser.add_argument("-m", "--class_map", help="Class mapping json file location.")
    parser.add_argument("-sm", "--set_map", help="Use set label from the class map. Defaults to false.", action='store_true')
    parser.add_argument("-ln", "--line_nums", help="Create files with the selected lines numbers for each set.", action='store_true')
    parser.add_argument("-da", "--dump_all", help="Dump all functions, not just the mapped ones.", action='store_true')
    parser.add_argument("-lbl", "--label", help="Label tag, used for equal splitting", default='label')
    parser.add_argument("-af", "--add_function", help="Adds full function body. The input file must have file_path, start_line, end_line specified. The result file overwrites the original.", action='store_true')
    parser.add_argument("-cfp", "--convert_file_path", help="Converts the file_path to the local file path. The input file must have file_path which has 'sources' as a root of all projects. The result file overwrites the original.", action='store_true')
    parser.add_argument("-osl", "--orig_source_location", help="The C/C++ sources location at the origin. Used for converting file path.")
    args = parser.parse_args()
    print(args)
    if args.convert_file_path:
        if args.orig_source_location is None:
            print("Please specify the original source location, like -osl C:\\Users\\Or\\Desktop\\Fatal-Library\\sources")
            exit(1)
        local_path(args.orig_source_location, args.location, args.jsonl_location)
    if args.add_function:
        add_function_body(args.location, args.jsonl_location, args.class_map)
    if not args.no_parse:
        parse_sources(args.location, args.jsonl_location, args.max_lines, args.class_map, args.set_map, args.dump_all)
    if args.split:
        split_dataset(args.jsonl_location, args.train_ratio, args.test_ratio, args.set_map, args.line_nums, args.label, args.class_map)

# dump_functions("/mnt/d/GitHub_Clones/scripts/C_Dataset/vlc/src/test/shared_data_ptr.cpp"
#     # sys.argv[1]
#     )
