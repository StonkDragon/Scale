#!env python3
import os
import shutil
from platform import system as osName

HOME = os.path.expanduser("~")

SCALE_INSTALL_DIR = os.path.join(HOME, "Scale")
SCALE_DATA_DIR = os.path.join(os.path.curdir, "Scale")
VERSION = "3.4"

build_command = f"g++ -Wall -Werror -pedantic -std=gnu++17 "
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

if osName() == 'Windows':
    build_command += "-static-libgcc -static-libstdc++ -static -lpthread "
    SCALE_INSTALL_DIR = "C:\\Windows\\sclc.exe"
elif osName() == 'Darwin':
    build_command += ""
    SCALE_INSTALL_DIR = "/usr/local/bin/sclc"
elif osName() == 'Linux':
    build_command += ""
    SCALE_INSTALL_DIR = "/usr/local/bin/sclc"
else:
    OSError(f"Unknown OS detected: {osName()}")

for unit in units:
    build_command += f"Compiler/{unit} "

try:
    os.mkdir("target")
except OSError:
    pass

build_command += "-o target/sclc -DVERSION=\\\"{VERSION}\\\""

print(build_command)
os.system(build_command)

shutil.move("target/sclc", SCALE_INSTALL_DIR)
