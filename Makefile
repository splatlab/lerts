TARGETS=main streamdump

ifdef D
	DEBUG=-g -DDEBUG_MODE
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

ifdef G
	GREEDY=-DGREEDY
else
	GREEDY=
endif

ifdef V
	VALIDATE=-DVALIDATE
else
	VALIDATE=
endif

ifdef P
	PROFILE=-pg -no-pie # for bug in gprof.
endif

CXX = g++ -std=c++11
CC = gcc -std=gnu11
LD= g++ -std=c++11

LOC_INCLUDE=include
LOC_SRC=src
OBJDIR=obj
LOGDIR=logs

CXXFLAGS += -Wall $(DEBUG) $(PROFILE) $(OPT) $(ARCH) $(GREEDY) $(VALIDATE) \
						-m64 -I. -I$(LOC_INCLUDE)

CFLAGS += -Wall $(DEBUG) $(PROFILE) $(OPT) $(ARCH) $(GREEDY) $(VALIDATE) \
					-m64 -I. -I$(LOC_INCLUDE)

LDFLAGS += $(DEBUG) $(PROFILE) $(OPT) -lpthread -lssl -lcrypto -lm

#
# declaration of dependencies
#

all: $(TARGETS)

# dependencies between programs and .o files

main:						$(OBJDIR)/main.o $(OBJDIR)/popcornfilter.o \
								$(OBJDIR)/cascadefilter.o $(OBJDIR)/zipf.o \
								$(OBJDIR)/gqf.o $(OBJDIR)/gqf_file.o $(OBJDIR)/util.o \
								$(OBJDIR)/hashutil.o $(OBJDIR)/partitioned_counter.o

streamdump: $(OBJDIR)/streamdump.o $(OBJDIR)/gqf.o $(OBJDIR)/util.o \
								$(OBJDIR)/hashutil.o

test: $(LOGDIR)
	./main popcornfilter -f 1 -q 16 -l 3 -g 2 -t 1 -a 1 -o -v 24

# dependencies between .o files and .h files

$(OBJDIR)/main.o: 						$(LOC_INCLUDE)/popcornfilter.h
$(OBJDIR)/popcornfilter.o: 		$(LOC_INCLUDE)/popcornfilter.h \
 															$(LOC_INCLUDE)/cascadefilter.h
$(OBJDIR)/cascadefilter.o: 		$(LOC_INCLUDE)/cascadefilter.h \
															$(LOC_INCLUDE)/gqf_cpp.h \
 															$(LOC_INCLUDE)/util.h \
 															$(LOC_INCLUDE)/lock.h \
 															$(LOC_INCLUDE)/partitioned_counter.h \
 															$(LOC_INCLUDE)/zipf.h
$(OBJDIR)/streamdump.o: 	$(LOC_INCLUDE)/gqf_cpp.h \
 															$(LOC_INCLUDE)/util.h \
 															$(LOC_INCLUDE)/lock.h

# dependencies between .o files and .cc (or .c) files

$(OBJDIR)/gqf.o: $(LOC_SRC)/gqf/gqf.c $(LOC_INCLUDE)/gqf/gqf.h
$(OBJDIR)/gqf_file.o: 	$(LOC_SRC)/gqf/gqf_file.c $(LOC_INCLUDE)/gqf/gqf_file.h
$(OBJDIR)/hashutil.o: 	$(LOC_INCLUDE)/gqf/hashutil.h
$(OBJDIR)/zipf.o: $(LOC_SRC)/zipf.c $(LOC_INCLUDE)/zipf.h
$(OBJDIR)/partitioned_counter.o: $(LOC_SRC)/partitioned_counter.c \
																$(LOC_INCLUDE)/partitioned_counter.h

#
# generic build rules
#

$(TARGETS):
	$(LD) $^ $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(LOC_SRC)/%.cc | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR)/%.o: $(LOC_SRC)/%.c | $(OBJDIR)
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR)/%.o: $(LOC_SRC)/gqf/%.c | $(OBJDIR)
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(LOGDIR):
	@mkdir -p $(LOGDIR)

clean:
	rm -rf $(OBJDIR) core $(TARGETS)
