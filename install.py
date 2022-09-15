#!env python3
import os
import shutil
import sys

HOME = os.path.expanduser("~")
PWD = os.path.curdir

SCALE_INSTALL_DIR = os.path.join(HOME, "Scale")
SCALE_DATA_DIR = os.path.join(PWD, "Scale")

euid = os.geteuid()

if euid != 0:
    if os.name != 'nt':
        try:
            os.remove(SCALE_INSTALL_DIR)
        except OSError:
            pass

        file_names = os.listdir(SCALE_DATA_DIR)
            
        for file_name in file_names:
            shutil.move(os.path.join(SCALE_DATA_DIR, file_name), HOME)
        
        print('Moving executable. This will require root privileges.')
        args = ['sudo', sys.executable] + sys.argv
        os.system(f'sudo python3 install.py')
        exit(0)
    else:
        print('On windows, this script must be run as Administrator')

else:
    if os.name == 'nt':
        try:
            os.remove(SCALE_INSTALL_DIR)
        except OSError:
            pass

        file_names = os.listdir(SCALE_DATA_DIR)
            
        for file_name in file_names:
            shutil.move(os.path.join(SCALE_DATA_DIR, file_name), HOME)

        os.remove("C:\\Windows\\sclc.exe")
        shutil.move(os.path.join(SCALE_DATA_DIR, "/sclc.exe"), "C:\\Windows")
    else:
        os.remove("/usr/local/bin/sclc")
        shutil.move("sclc", "/usr/local/bin")
