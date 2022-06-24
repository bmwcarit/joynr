#!/usr/bin/python3

import os
import sys
import xml.etree.ElementTree as ElementTree

bugCnt = 0

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
        if f.endswith("spotbugs.xml"):
            abspath = os.path.join(dirpath, f)
            func(abspath)
if (bugCnt >= 1):
    sys.exit(1)
sys.exit(0)
