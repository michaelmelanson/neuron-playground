set terminal aqua
set xrange [0:]
plot 'trace-1.txt' using 1:2 with lines title "membrane potential" #, 'trace-1.txt' using 1:3 with lines title "sodium/potassium channels", 'trace-1.txt' using 1:4 with lines title "other ion channels", 'trace-1.txt' using 1:5 with lines title "EPSP current"