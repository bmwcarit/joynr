import csv
from datetime import datetime
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

# create list of files
files = list()
fileNames = os.listdir(dir)
for fileName in fileNames:
    # only container files
    s = participantNamePattern.search(fileName)
    if not (s):
        continue
    files.append("{:s}/{:s}".format(dir, fileName))

# columns
firstDataColumn = 4  # from this column on, data will be correlated
timestampColumn = 3  # Timestamp column

# headers storage
headers = dict()

colAmount = 0

# correlate data by timestamps
data = dict()
for file in files:
    with open(file, "r") as inFile:
        csvLines = csv.reader(inFile)

        # save headers for later
        csvLine = next(csvLines)
        headers[file] = list()
        for idx in range(firstDataColumn, colAmount):
            headers[file].append(csvLine[idx])

        if colAmount == 0:
            colAmount = len(csvLine)
        else:
            assert(colAmount == len(csvLine))

        for csvLine in csvLines:
            # strip unnecessary precision
            timepoint = (csvLine[timestampColumn])[0:-9]

            # convert to python timepoint
            # string looks like this: 2021-06-01 14:53:41
            timepoint = datetime.strptime(timepoint, "%Y-%m-%d %H:%M:%S")

            if timepoint not in data:
                # create list of empty lists to hold the data
                data[timepoint] = list()
                for _ in files:
                    data[timepoint].append(list())
            # insert columns in list
            if data[timepoint][files.index(file)]:
                print("WARN: Ignoring duplicated datapoint in {:s} - [{:}]".format(file, timepoint.strftime("%Y-%m-%d %H:%M:%S")))
                continue
            for idx in range(firstDataColumn, colAmount):
                data[timepoint][files.index(file)].append(csvLine[idx])

#
# linearly interpolate missing datapoints
#

# create sorted list for going back and forth in the data set
sortedDataList = sorted(data)

# do this file by file
for fileIndex in range(0, len(files)):
    lastValues = list()
    lastIndex = 0
    for dataLineIndex in range(0, len(sortedDataList)):

        # no need for col by col as we only have or have not a complete line
        if not data[sortedDataList[dataLineIndex]][fileIndex]:
            if dataLineIndex == len(sortedDataList)-1:
                for idx in range(lastIndex, len(sortedDataList)):
                    for _ in range(0, colAmount - firstDataColumn):
                        pass
                        data[sortedDataList[idx]][fileIndex].append("0.0")
                continue
            # goto next line
            continue

        # fill beginning where no previous data exists
        if not lastValues:
            # found values but all before were invalid
            # beginning of data set
            for idx in range(lastIndex, dataLineIndex):
                for _ in range(0, colAmount - firstDataColumn):
                    data[sortedDataList[idx]][fileIndex].append("0.0")

        # missing values in between
        else:
            print("Interpolating {:s} to {:s} at {:s}".format(
                  str(sortedDataList[lastIndex]),
                  str(sortedDataList[dataLineIndex]),
                  files[fileIndex]))
            
            x0 = sortedDataList[lastIndex-1]      # Timepoint A
            x1 = sortedDataList[dataLineIndex]  # Timepoint B            
            for idx in range(lastIndex, dataLineIndex):
                x = sortedDataList[idx]
                delta1 = float((x1 - x).seconds)
                delta2 = float((x - x0).seconds)
                delta3 = float((x1 - x0).seconds)
                for col in range(0, colAmount - firstDataColumn):
                    y0 = float(data[sortedDataList[lastIndex-1]][fileIndex][col])      # Value A
                    y1 = float(data[sortedDataList[dataLineIndex]][fileIndex][col])  # Value B

                    y = (y0 * delta1 + y1 * delta2) / delta3
                    data[sortedDataList[idx]][fileIndex].append("{:.3f}".format(y))
            
        lastValues = data[sortedDataList[dataLineIndex]][fileIndex]
        lastIndex = dataLineIndex + 1


#
# print results
#

# for prettifying the header
namePattern = re.compile("(?<=_).*(?=(\.csv))")

# export correlated csv
outputFileName = "{:s}/{:s}".format(dir, "correlated.csv")
with open(outputFileName, "w") as outputFile:
    outputFile.write("timestamp")

    # print header
    # order of the list of lists should be consistent
    for file in files:
        for header in headers[file]:
            # try to prettify name
            name = namePattern.search(file)
            if name:
                name = name.group()
            else:
                name = file
            outputFile.write(",{:s}_{:s}".format(header, name))
    outputFile.write("\n")

    for timepoint in sorted(data):
        # convert timestamp back to string
        outputFile.write(timepoint.strftime("%Y-%m-%d %H:%M:%S"))

        for dataList in data[timepoint]:
            if not dataList:
                # missing datapoints should be interpolated now
                print("Error: Found missing datapoints that should have been interpolated")
                assert(False)
            else:
                if not len(dataList) == colAmount - firstDataColumn:
                    print("Error: Mismatch in output column count! File: {:s} - [{:s}]"
                        .format(file, timepoint.strftime("%Y-%m-%d %H:%M:%S")))
                    assert(False)
                for entry in dataList:
                    outputFile.write(",{:s}".format(entry))

        outputFile.write("\n")
