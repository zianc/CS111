
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
TARGET=lab0

# build the default executable
all : $(TARGET).c 
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

# run a simple smoke-test
check: all smoke_test

smoke_test: normal_case input_case output_case seg_case unrecognized_case
	if [ -s logfile.txt ]; then \
		echo "Smoke test failed with errors: "; \
		wc -l < logfile.txt; \
	fi; \
	rm -f myfile.txt logfile.txt 
normal_case:
	touch logfile.txt; \
	echo "hello, world" > myfile.txt; \
	cat myfile.txt | ./$(TARGET) > output.txt;  \
	if [ $$? -ne 0 ]; then \
		echo "Return value is incorrect for normal case." >> logfile.txt; \
	fi; \
	cmp -s myfile.txt output.txt > /dev/null; \
	if [ $$? -ne 0 ]; then \
		echo "Not successful copy." >> logfile.txt; \
	fi; \
	rm -f output.txt 
 	
input_case:
	./$(TARGET) --input myfile.txt > output.txt;  \
        if [ $$? -ne 0 ]; then \
                echo "Return value is incorrect for normal --input." >> logfile.txt; \
        fi; \
        cmp -s myfile.txt output.txt > /dev/null; \
        if [ $$? -ne 0 ]; then \
                echo "Not successful copy." >> logfile.txt; \
        fi; \
        rm -f output.txt; \
	./$(TARGET) --input soula.txt 2> message.txt; \
	if [ $$? -ne 2 ]; then \
		echo "Return value is incorrect for a nonexistent --input file." >> logfile.txt; \
	elif [ !  -s message.txt ]; then \
		echo "no error message with a nonexistent --input file." >> logfile.txt; \
	fi; \
	rm -f message.txt

output_case:
	 ./$(TARGET) --input myfile.txt --output output.txt;  \
        if [ $$? -ne 0 ]; then \
                echo "Return value is incorrect for normal --output." >> logfile.txt; \
        fi; \
        cmp -s myfile.txt output.txt > /dev/null; \
        if [ $$? -ne 0 ]; then \
                echo "Not successful copy." >> logfile.txt; \
        fi; \
        rm -f output.txt; 

seg_case:
	./$(TARGET) --segfault --catch 2> message.txt;  \
        if [ $$? -ne 4 ]; then \
                echo "Return value is incorrect for --segfault and --catch." >> logfile.txt; \
        elif [ !  -s message.txt ]; then \
                echo "no error message with caught segmentation fault." >> logfile.txt; \
	fi; \
	rm -f message.txt

unrecognized_case:
	./$(TARGET) --soula 2> message.txt;  \
        if [ $$? -ne 1 ]; then \
                echo "Return value is incorrect for unrecognized argument." >> logfile.txt; \
	elif [ !  -s message.txt ]; then \
                echo "no error message for unrecognized argument." >> logfile.txt; \
        fi; \
        rm -f message.txt






















