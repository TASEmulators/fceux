PREFIX  = 	/usr
OUTFILE = 	getSDLKey

CC	=	gcc
OBJS	=	getSDLKey.o
LIBS = -lSDL


all:		${OBJS}
		${CC} -o ${OUTFILE} ${LIBS} ${OBJS}

clean:
		rm -f ${OUTFILE} ${OBJS}

install:
		install -m 755 -D getSDLKey ${PREFIX}/bin/getSDLKey

getSDLKey.o = getSDLKey.cpp
