#set terminal pngcairo size 800,600 enhanced font 'Verdana,10'
#set output 'evolution_plot.png'
set encoding utf8
set title "Evolução do Fitness"
set grid
set xlabel "Geração"
set ylabel "Fitness"
set key top left

plot 'evolution_data.dat' using 1:2 with linespoints lw 2 pt 5 ps 0.5 title 'Melhor fitness', \
     ''                   using 1:3 with linespoints lw 2 pt 7 ps 0.75 title 'Fitness média'

pause -1 "Feche a janela criada para sair"
unset output
unset terminal
