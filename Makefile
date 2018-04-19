all:
	gcc server.c -pthread -o server
	gcc client.c -o client

server: server.o
	gcc server.c -pthread -o server

client: client.o
	gcc client.c -o client

clean:
	rm server client
