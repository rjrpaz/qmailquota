
CC = gcc

CFLAGS = -Wall -O2

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`

TARGET = mailquotacheck

all: clear $(TARGET)

mailquotacheck: mailquotacheck.c
#	indent -kr -ts4 mailquotacheck.c
	$(CC) $(CFLAGS) -o mailquotacheck mailquotacheck.c
	strip mailquotacheck

install:
	cp -a mailquotacheck /usr/local/bin/quotacheck

clear: 
	@clear

clean: 
	rm -f $(TARGET) *.o *.c~
