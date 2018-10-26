server: server.c command.o socketutils.o constants.o errorcode.h
	gcc server.c command.o socketutils.o constants.o -o server

client: client.c socketutils.o constants.o errorcode.h
	gcc client.c socketutils.o constants.o -o client

command.o: command.c command.h socketutils.h constants.h
	gcc -c command.c -o command.o

socketutils.o: socketutils.c socketutils.h errorcode.h
	gcc -c socketutils.c -o socketutils.o

constants.o: constants.c constants.h
	gcc -c constants.c -o constants.o

clean:
	rm *.o server client