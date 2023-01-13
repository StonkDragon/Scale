#!env python3
import os
import sys
import shutil
from platform import system as osName

if osName() == 'Windows':
    print("Scale can currently only be installed in WSL.")
    print("If this is already in WSL, follow manual build instructions.")
    exit(1)

if os.getuid() != 0:
    print("The install script must be run as root!")
    exit(1)

HOME = os.path.expanduser("~")
if HOME == "/root":
    print("Please specify your home directory with the '-home' flag!")
    exit(1)

compiler = "gcc"
cxxcompiler = "g++"
for i in range(1, len(sys.argv)):
    arg = sys.argv[i]
    if arg == "-with-compiler":
        i = i + 1
        if i < len(sys.argv):
            compiler = sys.argv[i]
        else:
            print("Expected argument for flag '-with-compiler'")
            exit(1)
    elif arg == "-home":
        i = i + 1
        if i < len(sys.argv):
            HOME = sys.argv[i]
        else:
            print("Expected argument for flag '-home'")
            exit(1)
    elif arg == "-cxx-compiler":
        i = i + 1
        if i < len(sys.argv):
            cxxcompiler = sys.argv[i]
        else:
            print("Expected argument for flag '-cxx-compiler'")
            exit(1)
    elif arg == "-help":
        print("Valid arguments are:")
        print("  -help                  Show this")
        print("  -with-compiler <comp>  Sets the compiler that 'sclc' will use to compile")
        print("  -home <path>           Overrides the install directory. This should always be your 'home' directory")
        print("  -cxx-compiler <comp>   Sets the compiler that this script will use to compile 'sclc'. Should be a C++ compiler")

SCALE_INSTALL_DIR = os.path.join(HOME, "Scale")
SCALE_DATA_DIR = os.path.join(os.path.curdir, "Scale")
VERSION = "23.0"

build_command = f"{cxxcompiler} -Wall -Werror -pedantic -std=gnu++17 "
units = [
    "Main.cpp",
    "modules/SyntaxTree.cpp",
    "modules/Parser.cpp",
    "modules/TokenHandlers.cpp",
    "modules/Tokenizer.cpp",
    "modules/Transpiler.cpp",
    "modules/DragonConfig.cpp",
    "modules/Function.cpp",
    "modules/InfoDumper.cpp",
    "Common.cpp"
]

build_command += ""
SCALE_INSTALL_DIR = "/usr/local/bin/sclc"

for unit in units:
    build_command += f"Compiler/{unit} "

try:
    os.mkdir("target")
except OSError:
    pass

build_command += "-o target/sclc -DVERSION=\\\"{VERSION}\\\" -DC_VERSION=\\\"gnu17\\\" -DCOMPILER=\\\"{compiler}\\\""

print(build_command)
os.system(build_command)
shutil.rmtree("Compiler")

print(f"Moving Executable to '{SCALE_INSTALL_DIR}'")
try:
    shutil.move("target/sclc", SCALE_INSTALL_DIR)
except:
    print(f"There was an error trying to move the exectuable to {SCALE_INSTALL_DIR}")
    print(f"Please move the file in 'target' manually to {SCALE_INSTALL_DIR}")
    input("Press enter once moved...")

print(f"Moving Files to '{SCALE_DATA_DIR}'")
shutil.move("Scale", SCALE_DATA_DIR)

print("Checking for VSCode installation")
if os.path.exists(f"{HOME}/.vscode"):
    print(f"Found VSCode installation in {HOME}/.vscode")
    shutil.move("scale.vscodeextension", f"{HOME}/.vscode")
elif os.path.exists(f"{HOME}/.vscode-insiders"):
    print(f"Found VSCode installation in {HOME}/.vscode-insiders")
    shutil.move("scale.vscodeextension", f"{HOME}/.vscode-insiders")
else:
    print("VSCode not found.")
    print("It is strongly suggested that you use VSCode for Scale Development.")
