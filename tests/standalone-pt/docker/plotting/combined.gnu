set term pngcairo size 3840,3840 font "Helvetica,25"
set datafile separator ','
set xdata time

dirPrefix = "./results/"

set output dirPrefix."/results.png"
containerFiles = system('python3 plotting/containers.py '.dirPrefix)

set multiplot layout 4,1

set timefmt "%Y-%m-%d %H:%M:%S"
set xlabel "Time [HH:MM:SS]"
set format x "%H:%M:%S"

#
# -----------------------------------------------------
#

set ylabel "Request rate [1/s]"
set title "Joynr Troughput (JEE app)"
plot 'results/results_backend.csv' using 2:4 title "Sent" with lines, \
     '' using 2:3 title "Received" with lines

#
# -----------------------------------------------------
#

set title "Joynr Latency (C++ apps)"
set ylabel "Latency [ms]"
set style fill transparent solid 0.2
plot for [i=1:words(containerFiles)] dirPrefix.word(containerFiles, i) \
            using 4:6:7 \
            title "" \
            with filledcurves, \
     for [i=1:words(containerFiles)] dirPrefix.word(containerFiles, i) \
            using 4:8 \
            title system('python3 plotting/prettyName.py '.word(containerFiles, i))  \
            with lines

#
# -----------------------------------------------------
#

set title "Joynr Latency (C++ apps) Scattered "
set ylabel "Latency [ms]"
plot for [i=1:words(containerFiles)] dirPrefix.word(containerFiles, i) \
            using 4:6 \
            title (i>1)?"":"max"  \
            with points pointtype 7 linecolor "light-red", \
     for [i=1:words(containerFiles)] dirPrefix.word(containerFiles, i) \
            using 4:8 \
            title (i>1)?"":"average"  \
            with points pointtype 7 linecolor "orange", \
     for [i=1:words(containerFiles)] dirPrefix.word(containerFiles, i) \
            using 4:7 \
            title (i>1)?"":"min"  \
            with points pointtype 7 linecolor "sea-green"

#
# -----------------------------------------------------
#

set style fill solid 1
set title "Joynr Throughput (C++ apps)"
set ylabel "Request rate [1/s]"

firstcol = 6
offset = 5
cumulated(i)=((i>firstcol)?column(i)+cumulated(i-offset):(i==firstcol)?column(i):1/0)
plot for [i=(words(containerFiles)-1):0:-1] dirPrefix."correlated.csv" \
            using 1:(cumulated(firstcol+offset*i)) \
            title system('python3 plotting/prettyName.py '.word(containerFiles, i+1)) \
            with filledcurves x1
