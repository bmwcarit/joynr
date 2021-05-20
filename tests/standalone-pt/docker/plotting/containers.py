import os
import re
import sys

# amount of cmd line args
if len(sys.argv) != 2:
    print("Missing path argument")
    exit(1)

# results directory
dir = sys.argv[1]
if not os.path.isdir(dir):
    print("unable to find directory: {:s}".format(dir))
    exit(1)

participantNamePattern = re.compile("cont_.*\.csv")

output = ""

files = os.listdir(dir)
for fileName in files:
    # only correct files
    s = participantNamePattern.search(fileName)
    if not (s):
        continue

    output += "{:s} ".format(fileName)

print(output)
