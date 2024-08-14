SRC=aesdsocket.c
TARGET=aesdsocket
OBJ=aesdsocket.o
CFLAGS= -g -Wall -Werror
CC=${CROSS_COMPILE}gcc
LIB=-lpthread -lrt
all : ${TARGET}

default : ${TARGET}

${TARGET} : ${OBJ}
		${CC} -o ${CFLAGS} ${OBJ} ${LIB} -o ${TARGET}	

${OBJ} : ${SRC}
		${CC} -c ${SRC} ${CFLAGS} -o ${OBJ}

clean :
		rm -rf aesdsocket.o aesdsocket

# all: aesdsocket

# aesdsocket.o: aesdsocket.c
# 	$(CC) $(CCFLAGS) -I . -c aesdsocket.c

# aesdsocket: aesdsocket.o
# 	$(CC) $(LDFLAGS) aesdsocket.o -lpthread -lrt -o aesdsocket

# clean :
# 	rm -f aesdsocket aesdsocket.o