#!/bin/bash
#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410

threads_1=(1 2 4 8 12 16 24)
iter_1=1000
syncs=(m s)

for i in "${threads_1[@]}"; do
	for j in "${syncs[@]}"; do
		./lab2_list --threads=$i --iterations=$iter_1 --sync=$j >> lab2b_list.csv
	done
done

threads_2=(1 4 8 12 16)
iters_2=(1 2 4 8 16)
list_2=4

for i in "${threads_2[@]}"; do
	for j in "${iters_2[@]}"; do
		./lab2_list --threads=$i --iterations=$j --yield=id --lists=$list_2 >> lab2b_list.csv
		for k in "${syncs[@]}"; do
			./lab2_list --threads=$i --iterations=$j --yield=id --sync=$k --lists=$list_2 >> lab2b_list.csv
		done
	done
done


threads_3=(1 2 4 8 12)
iters_3=1000
lists_3=(1 4 8 16)

for i in "${threads_3[@]}"; do
	for j in "${lists_3[@]}"; do
		for k in "${syncs[@]}"; do
			./lab2_list --threads=$i --iterations=$iters_3 --sync=$k --lists=$j >> lab2b_list.csv
		done
	done
done

