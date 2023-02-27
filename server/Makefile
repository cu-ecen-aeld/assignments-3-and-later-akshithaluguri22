CROSS_COMPILE=
CC=gcc
FLAGS= -g -Wall -Werror
SRC = aesdsocket.c
OUTPUT = aesdsocket

all:
	$(CROSS_COMPILE)$(CC) $(FLAGS) -o $(OUTPUT) $(SRC) 

clean:
	-rm -f *.o $(OUTPUT) *.elf *.map
