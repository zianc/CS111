#!/bin/bash

#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
# run test for range of threads and range of iterations without yield
threads=(1 2 4 8 12)
iterations=(1 10 100 1000 10000 100000)
syncAdd=(m s c)
for i in "${threads[@]}"; do
	for j in "${iterations[@]}"; do
		./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
	done
done	

# run test for range of threads and range of iterations with yield	
for i in "${threads[@]}"; do
	for j in "${iterations[@]}"; do
		./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
	done
done

# run test for range of threads and range of iterations with sync without yield	
for i in "${threads[@]}"; do
	for j in "${iterations[@]}"; do
		for k in "${syncAdd[@]}"; do
			./lab2_add --threads=$i --iterations=$j --sync=$k >> lab2_add.csv
		done
	done
done

# run test for range of threads and range of iterations with sync with yield
for i in "${threads[@]}"; do
	for j in "${iterations[@]}"; do
		for k in "${syncAdd[@]}"; do
			./lab2_add --threads=$i --iterations=$j --sync=$k --yield >> lab2_add.csv
		done
	done
done

list_iter_1=(10 100 1000 10000 20000)
list_thread_2=(2 4 8 12)
list_iter_2=(1 10 100 1000)
list_iter_3=(1 2 4 8 16 32)
yields=(i d l id il dl idl)
list_thread_3=(1 2 4 8 12 16 24)
syncList=(m s)
# run list test with a single thread with range of iterations
for i in "${list_iter_1[@]}"; do
	./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done	


for i in "${list_thread_2[@]}"; do
	for j in "${list_iter_2[@]}"; do
		./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
	done
done

for i in "${list_thread_2[@]}"; do
	for j in "${list_iter_3[@]}"; do
		for k in "${yields[@]}"; do
			./lab2_list --threads=$i --iterations=$j --yield=$k >> lab2_list.csv
		done
	done
done
for i in "${list_thread_2[@]}"; do
	for j in "${list_iter_3[@]}"; do
		for k in "${yields[@]}"; do
				./lab2_list --threads=$i --iterations=$j --yield=$k --sync=m >> lab2_list.csv
		done
	done
done
for i in "${list_thread_2[@]}"; do
	for j in "${list_iter_3[@]}"; do
		for k in "${yields[@]}"; do
				./lab2_list --threads=$i --iterations=$j --yield=$k --sync=s >> lab2_list.csv
		done
	done
done
for i in "${list_thread_3[@]}"; do
	for j in "${syncList[@]}"; do
		./lab2_list --threads=$i --sync=$j --iterations=1000 >> lab2_list.csv
	done
done	
