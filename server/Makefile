# SRC=aesdsocket.c
# TARGET=aesdsocket
# OBJ=aesdsocket.o
# CFLAGS= -g -Wall -Werror
# CC=${CROSS_COMPILE}gcc
# all : ${TARGET}

# default : ${TARGET}

# ${TARGET} : ${OBJ}
# 		${CC} -o ${CFLAGS} ${OBJ} -o ${TARGET}	

# ${OBJ} : ${SRC}
# 		${CC} -c ${SRC} ${CFLAGS} -o ${OBJ}

# clean :
# 		rm -rf aesdsocket.o aesdsocket

all: aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) $(CCFLAGS) -c aesdsocket.c

aesdsocket: aesdsocket.o
	$(CC) $(LDFLAGS) aesdsocket.o -o aesdsocket

clean :
	rm -f aesdsocket aesdsocket.o