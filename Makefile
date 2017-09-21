TARGETS=test main merge

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
CC = g++ -std=c++11
LD= g++ -std=c++11

CXXFLAGS += -Wall $(DEBUG) $(PROFILE) $(OPT) $(ARCH) -m64 -I. \
-Wno-unused-result -Wno-strict-aliasing -Wno-unused-function -Wno-sign-compare

LDFLAGS += $(DEBUG) $(PROFILE) $(OPT) -lpthread -lssl -lcrypto -lboost_system \
-lboost_thread -lm -lbz2 -lz

#
# declaration of dependencies
#

all: $(TARGETS)

# dependencies between programs and .o files

test:									 test.o									 util.o threadsafe-gqf/gqf.o
main:                  main.o 								 util.o hashutil.o threadsafe-gqf/gqf.o
merge:                 merge.o 								 util.o hashutil.o threadsafe-gqf/gqf.o

# dependencies between .o files and .h files

test.o:																		threadsafe-gqf/gqf.h
main.o: 								 									threadsafe-gqf/gqf.h hashutil.h util.h
merge.o: 								 									threadsafe-gqf/gqf.h hashutil.h util.h
hashutil.o: 																									 hashutil.h
util.o: 																									 								util.h

# dependencies between .o files and .cc (or .c) files

%.o: %.cc
threadsafe-gqf/gqf.o: threadsafe-gqf/gqf.c threadsafe-gqf/gqf.h util.h

#
# generic build rules
#

$(TARGETS):
	$(LD) $^ $(LDFLAGS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDE) $< -c -o $@

%.o: %.c
	$(CC) $(CXXFLAGS) $(INCLUDE) $< -c -o $@

clean:
	rm -f *.o core threadsafe-gqf/gqf.o $(TARGETS)
