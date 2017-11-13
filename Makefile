TARGETS=test main merge popcornfilter anomaly streamanalysis

ifdef D
	DEBUG=-g -DDEBUG
	OPT=
else
	DEBUG=
	OPT=-Ofast
endif

ifdef NH
	ARCH=
else
	ARCH=-msse4.2 -D__SSE4_2_
endif

ifdef P
	PROFILE=-pg -no-pie # for bug in gprof.
endif

CXX = g++ -std=c++11
CC = gcc -std=gnu11
LD= g++ -std=c++11

CXXFLAGS += -Wall $(DEBUG) $(PROFILE) $(OPT) $(ARCH) -m64 -I. \
-Wno-unused-result -Wno-strict-aliasing -Wno-unused-function -Wno-sign-compare

CFLAGS += -Wall $(DEBUG) $(PROFILE) $(OPT) $(ARCH) -m64 -I. \
-Wno-unused-result -Wno-strict-aliasing -Wno-unused-function -Wno-sign-compare \
-Wno-implicit-function-declaration

LDFLAGS += $(DEBUG) $(PROFILE) $(OPT) -lpthread -lssl -lcrypto -lboost_system \
-lboost_thread -lm -lbz2 -lz

#
# declaration of dependencies
#

all: $(TARGETS)

# dependencies between programs and .o files

test:					 				  test.o													cqf/gqf.o
main:                   main.o cascadefilter.o  zipf.o cqf/gqf.o util.o hashutil.o
merge:                  merge.o 											 cqf/gqf.o util.o hashutil.o
cascadefilter:         				 cascadefilter.o  zipf.o cqf/gqf.o util.o hashutil.o
popcornfilter: popcornfilter.o cascadefilter.o  zipf.o cqf/gqf.o util.o hashutil.o
anomaly:       anomaly.o                               cqf/gqf.o util.o hashutil.o
streamanalysis: streamanalysis.o                       cqf/gqf.o util.o hashutil.o

# dependencies between .o files and .h files

test.o:																		        cqf/gqf.h
main.o: 								         cascadefilter.h	cqf/gqf.h hashutil.h util.h zipf.h
merge.o: 								 									        cqf/gqf.h hashutil.h util.h
cascadefilter.o: 				         cascadefilter.h	cqf/gqf.h hashutil.h util.h zipf.h
popcornfilter.o: popcornfilter.h cascadefilter.h	cqf/gqf.h hashutil.h util.h zipf.h
hashutil.o: 																								hashutil.h
anomaly.o: 								 									        cqf/gqf.h hashutil.h util.h
streamanalysis.o: 								 							    cqf/gqf.h hashutil.h util.h

# dependencies between .o files and .cc (or .c) files

cqf/gqf.o: cqf/gqf.c cqf/gqf.h
zipf.o: zipf.c zipf.h

#
# generic build rules
#

$(TARGETS):
	$(LD) $^ $(LDFLAGS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) $< -c -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $< -c -o $@

clean:
	rm -f *.o core cqf/gqf.o $(TARGETS)
