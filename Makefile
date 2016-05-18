CC = gcc
CFLAGS = -Wall -Iinclude -g
LDFLAGS = -lpthread 

SOURCES = src/main.c src/cat.c src/touch.c src/cd.c src/copy.c src/command.c src/job.c
OBJS    = $(SOURCES:.c=.o)

all : $(SOURCES) system

system : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

*.c.o :
	$(CC) $(LDFLAGS) $< -o $@

clean:
	rm $(OBJS) system
