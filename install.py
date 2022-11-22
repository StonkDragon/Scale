#!env python3
import os
import shutil

HOME = os.path.expanduser("~")

SCALE_INSTALL_DIR = os.path.join(HOME, "Scale")
SCALE_DATA_DIR = os.path.join(os.path.curdir, "Scale")

if os.geteuid() != 0:
    print('This script must be run as root/Administrator')
    exit(1)

else:
    if os.name == 'nt':
        shutil.rmtree(SCALE_INSTALL_DIR)

        shutil.move(SCALE_DATA_DIR, HOME)

        os.remove("C:\\Windows\\sclc.exe")
        shutil.move(os.path.join(SCALE_DATA_DIR, "\\sclc.exe"), "C:\\Windows\\sclc.exe")
    else:
        shutil.rmtree(SCALE_INSTALL_DIR)
        
        shutil.move(SCALE_DATA_DIR, HOME)
        
        os.remove("/usr/local/bin/sclc")
        shutil.move("sclc", "/usr/local/bin/sclc")
