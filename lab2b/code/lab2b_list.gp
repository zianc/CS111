#! /usr/bin/gnuplot
#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410

#input: lab2b_list.csv

#general plot parameters
set terminal png
set datafile separator ","

# generate lab2b-1 that plots num of operations per second against num of threads
set title "List-1: Aggregate throughput of synchronization mechanisms"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops per sec)"
set output 'lab2b_1.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2): (1000000000/($7)) \
	title 'mutex_lock' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,' lab2b_list.csv" using ($2): (1000000000/($7)) \
	title 'spin_lock' with linespoints lc rgb 'green'

# generate lab2b-2 that plots average wait_for_clock time and time_per_operation against num of threads
set title "List-2: Wait_for_lock time analysis for mutex lock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Cost (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2): ($7) \
	title 'raw_time_per_op' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2): ($8) \
	title 'wait_for_lock' with linespoints lc rgb 'green'

# generate lab2b_3 that shows sucessful threads and iterations for protected and unprotected runs
unset logscale x
set title "List-3: Unprotected / Protected Threads and Iterations that run without failure"
set xlabel "Threads"
set xrange [0:20]
set ylabel "Successful Iterations"
set logscale y 2
set yrange [0.75:]
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep list-id-none, lab2b_list.csv" using ($2):($3) \
	title 'unprotected, yields=id' with points lc rgb 'red', \
     "< grep list-id-s, lab2b_list.csv" using ($2):($3) \
	title 'spin_locked, yield=id' with points lc rgb 'blue', \
     "< grep list-id-m, lab2b_list.csv" using ($2):($3) \
	title 'mutex_locked, yield=id' with points lc rgb 'green' 
#
# "no valid points" is possible if even a single iteration can't run
#

set title "List-4: Throughtput of mutex lock with partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set yrange [10000:]
set output 'lab2b_4.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/mutex, lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/mutex, lists=4' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/mutex, list=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/mutex, list=16' with linespoints lc rgb 'violet'


set title "List-5: Throughtput of spin lock with partitioned lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set yrange [10000:]
set logscale y 10
set output 'lab2b_5.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/spin_lock, lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/spin_lock, lists=4' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/spin_lock, list=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'w/spin_lock, list=16' with linespoints lc rgb 'violet'
