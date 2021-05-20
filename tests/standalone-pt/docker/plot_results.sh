
echo "interpolating missing data points"
python3 plotting/correlate.py ./results

echo "Plotting to ./results"
gnuplot plotting/combined.gnu

