CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRCDIR = src
BINDIR = bin

ALL_SRCS = $(SRCDIR)/server.c $(SRCDIR)/client.c
all: server client

server: $(SRCDIR)/server.c include/proto.h
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/server $(SRCDIR)/server.c

client: $(SRCDIR)/client.c include/proto.h
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/client $(SRCDIR)/client.c

clean:
	rm -rf $(BINDIR)

.PHONY: all server client clean
