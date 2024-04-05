
set xrange [0:10]
set yrange [0:200]
set title 'Live Data Plot' 
set xlabel 'Sample Number'
set ylabel 'Value'
plot 'dataFile.txt' using 1:2 w lp 
pause 1
reread