CROSS_COMPILE=
CC 	?= $(CROSS_COMPILE)gcc
CFLAGS	?= -g -Werror -Wall
LDFLAGS ?= -pthread -lrt
SRC = aesdsocket.c
OUTPUT = aesdsocket

all:
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)

clean:
	-rm -f *.o $(OUTPUT) *.elf *.map
