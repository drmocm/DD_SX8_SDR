CFLAGS =  -g  -Wno-unused -Wall -Wno-format -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
LIBS =  -lm  -lfftw3 
OBJ = ddsx8-spec.o numeric.o pam.o spec.o dvb.o blindscan.o
HEADER = numeric.h spec.h dvb.h pam.h blindscan.h
SRC = $(HEADER) numeric.c spec.c dvb.c blindscan.c
INCS = -I.

CC = gcc
CPP = g++

.PHONY: clean 

TARGETS = ddsx8-spec

all: $(TARGETS)

ddsx8-spec: $(OBJ) $(INC)
	$(CC) $(CFLAGS) -o ddsx8-spec $(OBJ) $(LIBS)

spec.o: $(HEADER) spec.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) spec.c

pam.o: $(HEADER) pam.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) pam.c

dvb.o: $(HEADER) dvb.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) dvb.c

blindscan.o: $(HEADER) blindscan.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) blindscan.c

ddsx8-spec.o: $(HEADER) ddsx8-spec.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) ddsx8-spec.c

clean:
	for f in $(TARGETS) *.o .depend *~ ; do \
		if [ -e "$$f" ]; then \
			rm "$$f" || exit 1; \
		fi \
	done

