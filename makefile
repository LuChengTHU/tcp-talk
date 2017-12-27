all:server client

CC=g++
CPPFLAGS=-Wall -std=c++11 -ggdb
LDFLAGS=-pthread

server:server.o
	$(CC) $(LDFLAGS) -o $@ $^
	rm server.o

server.o:server_folder/server.cpp
	$(CC) $(CPPFLAGS) -o $@ -c $^

client:client.o
	$(CC) $(LDFLAGS) -o $@ $^
	rm client.o

client.o:client_folder/client.cpp
	$(CC) $(CPPFLAGS) -o $@ -c $^


.PHONY:
	clean

clean:
	rm *.o server client