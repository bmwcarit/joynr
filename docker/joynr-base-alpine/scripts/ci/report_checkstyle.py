#!/usr/bin/python

import os
import sys
import xml.etree.ElementTree as ElementTree

bugCnt = 0

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
                print "*" * 80
                print "Checkstyle report"
            if (bugSeen == False):
                print "*" * 80
                print "name      =", name
                bugSeen = True
            print "*" * 80
            print "line       =", error.get('line')
            print "column     =", error.get('column')
            print "severity   =", error.get('severity')
            print "message    =", error.get('message')
            print "source     =", error.get('source')
            bugCnt += 1
        if (bugSeen):
            print "*" * 80

for dirpath, dirnames, filenames in os.walk("./"):
    for f in filenames:
        if f.endswith("checkstyle-result.xml"):
            abspath = os.path.join(dirpath, f)
            func(abspath)
if (bugCnt >= 1):
    sys.exit(1)
sys.exit(0)
