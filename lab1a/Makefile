#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
# A Makefile for project 1
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
TARGET=lab1a
UID=804757410

# build the default executable
default : $(TARGET).c 
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET) $(TARGET)-$(UID).tar.gz

dist: $(TARGET).c Makefile README
	tar -czvf $(TARGET)-$(UID).tar.gz $(TARGET).c Makefile README




