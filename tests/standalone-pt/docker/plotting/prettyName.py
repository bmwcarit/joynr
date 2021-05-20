import re
import sys

# amount of cmd line args
if len(sys.argv) != 2:
    print("Missing path argument")
    exit(1)

str = sys.argv[1]
namePattern = re.compile("(?<=_)[A-Za-z0-9]*(?=(\.csv))")

s = namePattern.search(str)
if s:
    print(s.group())
else:
    print(str)
