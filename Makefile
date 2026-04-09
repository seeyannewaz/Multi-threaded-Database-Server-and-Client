all: dbserver dbclient

clean:
	rm -f dbserver dbclient

dbserver: dbserver.o
	gcc dbserver.o -o dbserver -Wall -Werror -std=gnu99 -pthread

dbclient: dbclient.o
	gcc dbclient.o -o dbclient -Wall -Werror -std=gnu99

dbserver.o: dbserver.c msg.h
	gcc -c dbserver.c

dbclient.o: dbclient.c msg.h
	gcc -c dbclient.c
