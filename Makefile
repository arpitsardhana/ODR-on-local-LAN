
CC = gcc

LIBS = -lpthread -lm -lc \
	/users/cse533/Stevens/unpv13e/libunp.a

FLAGS =  -g3
CFLAGS = ${FLAGS} -I/users/cse533/Stevens/unpv13e/lib

all: server client odr

server: server.o utility.o  
	${CC} ${FLAGS} -o server server.o utility.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c
	
client: client.o utility.o
	${CC} ${FLAGS} -o client client.o utility.o  ${LIBS}

utility.o: utility.c
	${CC} ${CFLAGS} -c utility.c

odr: get_hw_addrs.o odr.o utility.o 
	${CC} ${FLAGS} -o $@ $^ ${LIBS}
odr.o: odr.c
	${CC} ${CFLAGS} -c -o $@ $^
	
get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c -o $@ $^
	
clean:
	rm -f server server.o client client.o utility.o odr odr.o get_hw_addrs.o core



