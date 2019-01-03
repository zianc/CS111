#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
	
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
LIBFLAGS = -lmraa -lm
TARGET=lab4b
UID=804757410

# build the default executable
default : $(TARGET).c 
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBFLAGS)
check: default
	echo OFF | ./$(TARGET) --period=5 --scale=C --log=soula.txt
	rm -f soula.txt

clean:
	rm -f $(TARGET) $(TARGET)-$(UID).tar.gz
dist: $(TARGET).c Makefile README
	tar -czvf $(TARGET)-$(UID).tar.gz $(TARGET).c Makefile README




