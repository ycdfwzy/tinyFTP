server: server.c command.o socketutils.o errorcode.h
	gcc server.c command.o socketutils.o -o server
client: client.c socketutils.o errorcode.h
	gcc client.c socketutils.o -o client
command.o: command.c command.h
	gcc -c command.c -o command.o
socketutils.o: socketutils.c socketutils.h errorcode.h
	gcc -c socketutils.c -o socketutils.o
clean:
	rm *.o server client