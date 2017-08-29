
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
	su -c "cp -a mailquotacheck /usr/local/bin/mailquotacheck; chown root.root /usr/local/bin/mailquotacheck; chmod 755 /usr/local/bin/mailquotacheck" root

clear: 
	@clear

clean: 
	rm -f $(TARGET) *.o *.c~
