CC=gcc
CFLAGS=-g -Wall -Werror

all: obssim_unpack

check: obssim_unpack
	-./obssim_unpack

#./obssim_unpack 1234 ./

clean:
	rm -rf obssim_unpack obssim_unpack.dSYM

obssim_unpack: obssim_unpack.c
	$(CC) $(CFLAGS) obssim_unpack.c -o $@
