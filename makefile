server: server.c command.o socketutils.o constants.o dataserver.o errorcode.h
	gcc server.c command.o socketutils.o constants.o dataserver.o -o server

client: client.c socketutils.o constants.o command.o dataclient.o errorcode.h
	gcc client.c socketutils.o constants.o command.o dataclient.o -o client

command.o: command.c dataserver.h command.h socketutils.h constants.h
	gcc -c command.c -o command.o

socketutils.o: socketutils.c socketutils.h errorcode.h
	gcc -c socketutils.c -o socketutils.o

constants.o: constants.c constants.h
	gcc -c constants.c -o constants.o

dataserver.o: dataserver.c dataserver.h errorcode.h
	gcc -c dataserver.c -o dataserver.o

dataclient.o: dataclient.c dataclient.h errorcode.h
	gcc -c dataclient.c -o dataclient.o

clean:
	rm *.o server client