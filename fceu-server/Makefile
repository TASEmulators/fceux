PREFIX  = 	/usr
OUTFILE = 	fceu-server

CC	=	g++
OBJS	=	server.o md5.o throttle.o


all:		${OBJS}
		${CC} -o ${OUTFILE} ${OBJS}

clean:
		rm -f ${OUTFILE} ${OBJS}

install:
		install -m 755 -D fceu-server ${PREFIX}/bin/fceu-server
		install -m 644 -D fceu-server.conf /etc/fceu-server.conf

server.o:	server.cpp
md5.o:		md5.cpp
throttle.o:	throttle.cpp
