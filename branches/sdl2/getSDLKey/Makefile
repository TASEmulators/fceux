PREFIX  = 	/usr
OUTFILE = 	getSDLKey

CC	=	g++
OBJS	=	getSDLKey.o
LIBS = $(shell sdl-config --cflags --libs)

all:		${OBJS}
		${CC} -o ${OUTFILE} ${OBJS} ${LIBS}

clean:
		rm -f ${OUTFILE} ${OBJS}

install:
		install -m 755 -D getSDLKey ${PREFIX}/bin/getSDLKey

getSDLKey.o = getSDLKey.cpp
