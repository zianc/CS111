#NAME: Haiying Huang
#EMAIL: hhaiying1998@outlook.com
#ID: 804757410
	
# define Makefile variables
CC=gcc
CFLAGS=-Wall -Wextra
LIBFLAGS =-lmraa -lm
TARGET=lab4c
UID=804757410

# build the default executable
default : $(TARGET)_tcp.c $(TARGET)_tls.c
	$(CC) $(CFLAGS) -o $(TARGET)_tcp $(TARGET)_tcp.c $(LIBFLAGS)
	$(CC) $(CFLAGS) -o $(TARGET)_tls $(TARGET)_tls.c $(LIBFLAGS) -lssl -lcrypto

test_tcp:
	./$(TARGET)_tcp --id=$(UID) --host=lever.cs.ucla.edu --log=soula.txt --scale=C 18000
	rm -f soula.txt
test_tls:
	./$(TARGET)_tls --id=$(UID) --host=lever.cs.ucla.edu --log=soula.txt --scale=C 19000
	rm -f soula.txt

test: default test_tcp test_tsl

clean:
	rm -f $(TARGET)_tcp $(TARGET)_tls $(TARGET)-$(UID).tar.gz
dist: $(TARGET)_tcp.c $(TARGET)_tls.c Makefile README
	tar -czvf $(TARGET)-$(UID).tar.gz $(TARGET)_tcp.c $(TARGET)_tls.c Makefile README




