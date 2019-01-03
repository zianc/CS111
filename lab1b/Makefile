#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
# A Makefile for project 1
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
TARGET=lab1b
TARGET1=lab1b-client
TARGET2=lab1b-server

UID=804757410

# build the default executable
default : $(TARGET1).c $(TARGET2).c 
	$(CC) $(CFLAGS) -o $(TARGET1) $(TARGET1).c -lmcrypt 
	$(CC) $(CFLAGS) -o $(TARGET2) $(TARGET2).c -lmcrypt

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET)-$(UID).tar.gz

dist: $(TARGET1).c $(TARGET2).c Makefile README my.key
	tar -czvf $(TARGET)-$(UID).tar.gz $(TARGET1).c $(TARGET2).c my.key Makefile README




