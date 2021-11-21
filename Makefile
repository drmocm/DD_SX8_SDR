CFLAGS =  -g  -Wno-unused -Wall -Wno-format -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
LIBS =  -lm  -lfftw3 
DDSX8OBJ = ddsx8-spec.o numeric.o pam.o spec.o dvb.o blindscan.o iod.o
PAMOBJ = pam_test.o pam.o
PARAMOBJ = dd_param_zap.o dvb.o iod.o 
HEADER = numeric.h spec.h dvb.h pam.h blindscan.h iod.h
SRC = $(HEADER) numeric.c spec.c dvb.c blindscan.c
INCS = -I.

CC = gcc
CPP = g++

.PHONY: clean 

TARGETS = ddsx8-spec pam_test dd_param_zap

all: $(TARGETS)

ddsx8-spec: $(DDSX8OBJ) $(INC)
	$(CC) $(CFLAGS) -o ddsx8-spec $(DDSX8OBJ) $(LIBS)

pam_test: $(PAMOBJ) pam.h
	$(CC) $(CFLAGS) -o pam_test $(PAMOBJ) $(LIBS)

dd_param_zap: $(PARAMOBJ) iod.h dvb.h 
	$(CC) $(CFLAGS) -o dd_param_zap $(PARAMOBJ) $(LIBS)


iod.o: $(HEADER) iod.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) iod.c

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

pam_test.o: $(HEADER) pam_test.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) pam_test.c

dd_param_zap.o: $(HEADER) dd_param_zap.c
	$(CC) -c $(CFLAGS) $(INCS) $(DEFINES) dd_param_zap.c


clean:
	for f in $(TARGETS) *.o .depend *~ ; do \
		if [ -e "$$f" ]; then \
			rm "$$f" || exit 1; \
		fi \
	done

