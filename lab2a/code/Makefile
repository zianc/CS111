#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
TARGET=lab2
UID=804757410
# build the default executable

build: build_add build_list

build_add: $(TARGET)_add.c
	$(CC) $(CFLAGS) -o $(TARGET)_add $(TARGET)_add.c -lpthread -lrt
build_list: $(TARGET)_list.c SortedList.c SortedList.h
	$(CC) $(CFLAGS) -o $(TARGET)_list SortedList.c $(TARGET)_list.c -lpthread -lrt

tests: build
	rm -f $(TARGET)_add.csv $(TARGET)_list.csv
	chmod +x test.sh
	-./test.sh 2>/dev/null
graphs: $(TARGET)_add.csv $(TARGET)_list.csv
	chmod +x $(TARGET)_add.gp $(TARGET)_list.gp
	./$(TARGET)_add.gp 2> /dev/null
	./$(TARGET)_list.gp 2> /dev/null
clean:
	rm -f $(TARGET)_add $(TARGET)_list $(TARGET)a-$(UID).tar.gz *.gch
dist: tests graphs clean
	tar -czvf $(TARGET)a-$(UID).tar.gz $(TARGET)_add.c $(TARGET)_list.c SortedList.h SortedList.c $(TARGET)_add.csv $(TARGET)_list.csv Makefile test.sh $(TARGET)_add.gp $(TARGET)_list.gp README *.png




