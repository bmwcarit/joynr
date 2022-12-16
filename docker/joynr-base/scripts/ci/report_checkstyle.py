#!/usr/bin/python3

import os
import sys
import xml.etree.ElementTree as ElementTree

bugCnt = 0
resultCnt = 0
once = False

def func(abspath):
    global bugCnt
    tree = ElementTree.parse(abspath)
    root = tree.getroot()
    files = root.findall('file')
    for file in files:
        bugSeen = False
        name = file.get('name')
        errors = file.findall('error')
        for error in errors:
            if (bugCnt == 0):
                print("*" * 80)
                print("Checkstyle report")
            if (bugSeen == False):
                print("*" * 80)
                print("name      =", name)
                bugSeen = True
            print("*" * 80)
            print("line       =", error.get('line'))
            print("column     =", error.get('column'))
            print("severity   =", error.get('severity'))
            print("message    =", error.get('message'))
            print("source     =", error.get('source'))
            bugCnt += 1
        if (bugSeen):
            print("*" * 80)

for dirpath, dirnames, filenames in os.walk("./"):
    for f in filenames:
        if (once == False):
            once = True
            print("report_checkstyle.py: " + os.path.join(dirpath, f) + "\n dirnames: " + str(dirnames))
        if f.endswith("checkstyle-result.xml"):
            resultCnt += 1
            abspath = os.path.join(dirpath, f)
            func(abspath)
print("report_checkstyle.py: " + os.path.abspath(os.path.dirname(__file__)))
print("report_checkstyle.py cwd: " + os.getcwd())
print("Found " + str(resultCnt) + " checkstyle result files")
if (bugCnt >= 1 or resultCnt < 1):
    sys.exit(1)
sys.exit(0)
