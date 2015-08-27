VPATH=.:../obssim_unpack
CC=gcc
CFLAGS=-g -Wall -Werror -O3

all: obssim2fits tess_obssim

#
# CFITSIO
#
CFITSIO_DIR = /usr/local
CFITSIO_INC = -I${CFITSIO_DIR}/include
CFITSIO_LIB =   ${CFITSIO_DIR}/lib
CFITSIO_LNK = -L${CFITSIO_LIB} -lcfitsio
#./obssim_unpack 1234 .
INCS	= -I../tess_camlink -I../obssim_unpack ${CFITSIO_INC}
LIBS	= ${CFITSIO_LNK} -lpthread -lm -ldl

.c.o:
	${CC} ${CFLAGS} ${INCS} -c $< -o $@

clean:
	rm -rf *.o obssim2fits tess_obssim *~


obssim2fits: obssim2fits.o tess_utils.o tess_fits.o tess_notes.o gps.o 
	$(CC) $(CFLAGS) $^ -o $@ ${LIBS}

tess_obssim: tess_obssim.o obssim_unpack.o tess_housekeeping.o tess_guidestamps.o tess_utils.o tess_fits.o tess_notes.o gps.o nthi.o
	$(CC) $(CFLAGS) $^ -o $@ ${LIBS}


readHK: readHK.o  obssim_unpack.o tess_housekeeping.o gps.o
	$(CC) $(CFLAGS) $^ -o $@ ${LIBS}


test_housekeeping: test_housekeeping.o tess_housekeeping.o
	$(CC) $(CFLAGS) $^ -o $@ ${LIBS}
