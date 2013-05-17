
PROG	=	imgcat
VER		=	1.0
CFLAGS	+=	`imlib2-config --cflags`
LIBS	+= 	`imlib2-config --libs`
PREFIX	?=	/usr

${PROG}: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS} ${LIBS}
	@strip ${PROG}

clean:
	@rm -f ${PROG}

tarball: clean
	@rm -f ${PROG}-${VER}.tar.gz
	@tar -czf ${PROG}-${VER}.tar.gz *

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}




