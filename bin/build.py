"""
This script handles our game engine's build workflow.

- Find source files
- Compile executable using clang
- Run executable
"""

import glob  # For finding files
import os    # For system calls
import sys   # For command line arguments

ENGINE = "OURBINARY"
CLANG = "clang.exe"

def findFiles(searchPath, searchExtension, exclude=None):
    """
    Find all files that end with the specified extension
    in the specified path
    """
    if not exclude:
        exclude = []
    elif not isinstance(exclude, list):
        exclude = [exclude]
    
    globPattern = f"{searchPath}**{os.sep}*.{searchExtension}"
    fileList = glob.glob(globPattern, recursive=True)
    
    finalFileList = [x for x in fileList]
    for x in fileList:
        for excludePattern in exclude:
            if excludePattern in x:
                finalFileList.remove(x)
    return finalFileList

    # Shorthand
    # return [x for x in fileList if not any([x.endswith(y) for y in exclude])]

    

def clang(filename, cStandard, 
          *, 
          files=None, 
          flags=None, 
          libraries=None, 
          linkDirs=None, 
          includeDirs=None, 
          compileCommands=False):
    """
    Compiles our project using clang (msvc wrapper)
    """
    # Example Command:
    # clang -MJ compile_commands.json -std=c11 -g -o <FILENAME>.exe -I ../include -I ../src/ <file1.c> <file2.c> <file3.c>
    if not files or len(files) == 0:
        print("No files to compile!")
        return
    
    command = (
        f"{CLANG} "
        f"{'-MJ compile_commands.json' if compileCommands else ''} "
        f"-std=\"{cStandard}\" "
        f"{' '.join([f for f in flags]) if flags else ''} "
        f"-g -o {filename}.exe "
        f"{' '.join(['-l ' + p for p in libraries]) if libraries else ''} "
        f"{' '.join(['-L ' + p for p in linkDirs]) if linkDirs else ''} "
        f"{' '.join(['-I ' + p for p in includeDirs]) if includeDirs else ''} "
        f"{' '.join([f for f in files]) if files else ''}"
    )
    
    print(command)
    os.system(command)
    
    if compileCommands:
        with open("compile_commands.json", "r") as rf:
            with open("../compile_commands.json", "w") as wf:
                wf.write("[\n")
                wf.write(rf.read().strip()[:-1])
                wf.write("\n]")
                
def main():
    filesToCompile = findFiles(f"..{os.sep}src{os.sep}", "c", exclude=["test.c"])
    clang(ENGINE, "c11", files=filesToCompile, includeDirs=["../src", "../include"])
    
    if "--run" in sys.argv:
        os.system(f"..{os.sep}build{os.sep}{ENGINE}")

if __name__ == '__main__':
    main()