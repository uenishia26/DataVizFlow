
set xrange [0:40]
set yrange [0:200]
set title 'Live Data Plot' 
set xlabel 'Sample Number'
set ylabel 'Value'
plot 'dataFile.txt' using 1:2 w lp 
pause 0.1
reread
