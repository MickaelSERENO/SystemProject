CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = 

SOURCES = src/main.c src/cat.c src/touch.c src/cd.c
OBJS    = $(SOURCES:.c=.o)

all : $(SOURCES) system

system : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

*.c.o :
	$(CC) $(LDFLAGS) $< -o $@

clean:
	rm $(OBJS) system
