set datafile separator ","
plot 'test.csv' using 1:2 axes x1y1  with lines,\
     'test.csv' using 1:3 axes x1y2  with lines

pause -1