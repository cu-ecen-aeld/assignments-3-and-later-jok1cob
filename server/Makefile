SRC=aesdsocket.c
TARGET=aesdsocket
OBJ=aesdsocket.o
CFLAGS= -g -Wall -Werror
CC=${CROSS_COMPILE}gcc
all : ${TARGET}

default : ${TARGET}

${TARGET} : ${OBJ}
		${CC} -o ${CFLAGS} ${OBJ} -o ${TARGET}	

${OBJ} : ${SRC}
		${CC} -c ${SRC} ${CFLAGS} -o ${OBJ}

clean :
		rm -rf aesdsocket.o aesdsocket