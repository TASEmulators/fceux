PREFIX  ?= 	/usr
OUTFILE = 	fceux-net-server

CXX	?=	g++
OBJS	=	server.o md5.o throttle.o


all:		${OBJS}
		${CXX} ${CXXFLAGS} -o ${OUTFILE} ${OBJS} ${LDFLAGS}

clean:
		rm -f ${OUTFILE} ${OBJS}

install:
		install -m 755 -D fceux-net-server ${PREFIX}/bin/fceux-server
		install -m 644 -D fceux-server.conf /etc/fceux-server.conf

server.o:	server.cpp
md5.o:		md5.cpp
throttle.o:	throttle.cpp
