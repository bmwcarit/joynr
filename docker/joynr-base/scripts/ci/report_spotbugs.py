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
    file = root.find('file')
    if (file == None):
        return
    if (bugCnt == 0):
        print("*" * 80)
        print("Spotbugs report")
    classname = file.get('classname')
    print("*" * 80)
    print("class      =", classname)
    bugInstances = file.findall('BugInstance')
    for bugInstance in bugInstances:
        print("*" * 80)
        print("category   =", bugInstance.get('category'))
        print("lineNumber =", bugInstance.get('lineNumber'))
        print("message    =", bugInstance.get('message'))
        print("priority   =", bugInstance.get('priority'))
        print("type       =", bugInstance.get('type'))
        bugCnt += 1
    print("*" * 80)

for dirpath, dirnames, filenames in os.walk("./"):
    for f in filenames:
        if (once == False):
            once = True
            print("report_spotbugs.py: " + os.path.join(dirpath, f) + "\n dirnames: " + str(dirnames))
        if f.endswith("spotbugs.xml"):
            resultCnt += 1
            abspath = os.path.join(dirpath, f)
            func(abspath)
print("report_spotbugs.py: " + os.path.abspath(os.path.dirname(__file__)))
print("report_spotbugs.py cwd: " + os.getcwd())
print("Found " + str(resultCnt) + " spotbugs result files")
if (bugCnt >= 1 or resultCnt < 1):
    sys.exit(1)
sys.exit(0)
