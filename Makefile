LDFLAGS=-L.
LDLIBS=-lPiPack -lrt -lm -lncurses -pthread -lwiringPi
LDLIBS2=-lPiPack2 -lrt -lm -lncurses -pthread -lwiringPi
LDLIBS3=-lLedGrid -lrt -lm -lncurses -pthread -lwiringPi
# CFLAGS=-DNDEBUG
# CFLAGS=-DNDEBUG -g -pg -ggdb
# CFLAGS=-ggdb -DNDEBUG -pg -O

libPiPack.so: PiPack.c PiPack.h
	${CC} ${CFLAGS} -c -o PiPack.o $<
	${LD} -r -o $@ PiPack.o

libPiPack2.so: PiPack2.c PiPack2.h
	${CC} ${CFLAGS} -c -o PiPack2.o $<
	${LD} -r -o $@ PiPack2.o

libLedGrid.so: LedGrid.c LedGrid.h
	${CC} ${CFLAGS} -c -o LedGrid.o $<
	${LD} -r -o $@ LedGrid.o

%: %.c libPiPack.so

spiTest: spiTest.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $< -lwiringPi

ledgrid2_01: ledgrid2_01.c libPiPack2.so
	${CC} ${CFLAGS} ${LDFLAGS} -o ledgrid2_01 ledgrid2_01.c ${LDLIBS2}

ledstrip2_01: ledstrip2_01.c libPiPack2.so
	${CC} ${CFLAGS} ${LDFLAGS} -o ledstrip2_01 ledstrip2_01.c ${LDLIBS2}

ledgrid_new_01: ledgrid_new_01.c libLedGrid.so
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $< ${LDLIBS3}

ledgrid_new_02: ledgrid_new_02.c libLedGrid.so
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $< ${LDLIBS3}

ledgrid_new_03: ledgrid_new_03.c libLedGrid.so
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $< ${LDLIBS3}

ledgrid_new_07: ledgrid_new_07.c libLedGrid.so
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $< ${LDLIBS3}

