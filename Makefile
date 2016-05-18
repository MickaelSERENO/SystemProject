CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS = -lpthread 

SOURCES = src/main.c src/cat.c src/touch.c src/cd.c src/copy.c
OBJS    = $(SOURCES:.c=.o)

all : $(SOURCES) system

system : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

*.c.o :
	$(CC) $(LDFLAGS) $< -o $@

clean:
	rm $(OBJS) system
