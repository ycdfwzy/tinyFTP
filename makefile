server: server.c command.o socketutils.o constants.o datautils.o errorcode.h
	gcc server.c command.o socketutils.o constants.o datautils.o -o server -W -Wall

client: client.c socketutils.o constants.o command.o datautils.o errorcode.h
	gcc client.c socketutils.o constants.o command.o datautils.o -o client -W -Wall

command.o: command.c datautils.h command.h socketutils.h constants.h
	gcc -c command.c -o command.o -W -Wall

socketutils.o: socketutils.c socketutils.h errorcode.h
	gcc -c socketutils.c -o socketutils.o -W -Wall

constants.o: constants.c constants.h
	gcc -c constants.c -o constants.o -W -Wall

datautils.o: datautils.c datautils.h errorcode.h socketutils.h
	gcc -c datautils.c -o datautils.o -W -Wall

clean:
	rm *.o server client