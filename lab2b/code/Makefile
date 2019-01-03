#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
TARGET=lab2
UID=804757410
LIB=/u/ee/ugrad/haiying/cs111/project2b/gperftools/
# build the default executable

default: $(TARGET)_list.c SortedList.c SortedList.h
	$(CC) $(CFLAGS) -o $(TARGET)_list SortedList.c $(TARGET)_list.c -lpthread -lrt

tests: default
	@rm -f $(TARGET)b_list.csv
	@chmod +x test.sh
	-./test.sh
graphs: $(TARGET)b_list.csv
	chmod +x $(TARGET)b_list.gp
	./$(TARGET)b_list.gp
profile:
	$(CC) $(CFLAGS) -o $(TARGET)_list SortedList.c $(TARGET)_list.c -L$(LIB)/lib -lprofiler
	LD_PRELOAD=$(LIB)/lib/libprofiler.so.0  CPUPROFILE=./myfile.gperf ./$(TARGET)_list --threads=12 --iterations=1000 --sync=s
	./$(LIB)/bin/pprof --text $(TARGET)_list myfile.gperf > profile.out
	./$(LIB)/bin/pprof --list=myThreadFunc $(TARGET)_list myfile.gperf >> profile.out


clean:
	rm -f $(TARGET)_list $(TARGET)b-$(UID).tar.gz
dist: tests graphs clean
	tar -czvf $(TARGET)b-$(UID).tar.gz $(TARGET)_list.c SortedList.h SortedList.c $(TARGET)b_list.csv $(TARGET)b_list.gp Makefile test.sh profile.out README *.png

temp:
	tar -czvf $(TARGET)b-$(UID).tar.gz $(TARGET)_list.c SortedList.h SortedList.c $(TARGET)b_list.csv $(TARGET)b_list.gp Makefile test.sh profile.out README *.png


