CC=cc
CFLAGS=-pedantic -Wall -Werror
RM=rm -fv
.PHONY: all clean clean_all
all: send_file_gba.out
%.o: %.c %.h
		$(CC) $(CFLAGS) -c -o $@ $<
send_file_gba.out: send_file_gba.c serial.o
		$(CC) $(CFLAGS) -o $@ $^
clean:
		$(RM) *.o *.out
