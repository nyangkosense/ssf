# ssf - simple system fetcher

include config.mk

SRC = ssf.c
OBJ = ${SRC:.c=.o}
BIN = ssf
MAN1 = ssf.1
MANDIR = ${MANPREFIX}

all: options ${BIN}

options:
	@echo "ssf build options:"
	@echo "CC       = ${CC}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"

${BIN}: ${OBJ}
	${CC} ${OBJ} ${LDFLAGS} -o $@

%.o: %.c ascii
	${CC} ${CPPFLAGS} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${BIN} ${OBJ}

install: all
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 755 ${BIN} ${DESTDIR}${BINDIR}/${BIN}
	mkdir -p ${DESTDIR}${MANDIR}/man1
	install -m 644 ${MAN1} ${DESTDIR}${MANDIR}/man1/${MAN1}

uninstall:
	rm -f ${DESTDIR}${BINDIR}/${BIN}
	rm -f ${DESTDIR}${MANDIR}/man1/${MAN1}

.PHONY: all options clean install uninstall
