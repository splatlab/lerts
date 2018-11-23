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

setup:
	sudo apt install cgroup-tools
	sudo cgconfigparser -l scripts/cgconfig.conf

firehose:
	git clone https://github.com/splatlab/firehose.git
	cd firehose/generators/active/; make; cd -

data_gen50M:
	./streamdump -s 50000000 -f raw/streamdump_mmap_active_new_50M 12345 & \
	./firehose/generators/active/active -n 1000000 -r 500000 -a 1048576 127.0.0.1@12345

popcorn50M:
	echo "Scaling throughput for 50M dataset" > 50M_data.outpput
	echo 33554432 > /var/cgroups/popcorning/memory.limit_in_bytes
	echo "-f 8 -q 19 -l 3 -g 4 -t 1 -o -e -v 24" >> 50M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 8 -q 19 -l 3 -g 4 -t 1 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_50M >> 50M_data.output
	echo "-f 8 -q 19 -l 3 -g 4 -t 2 -o -e -v 24" >> 50M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 8 -q 19 -l 3 -g 4 -t 2 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_50M >> 50M_data.output
	echo "-f 8 -q 19 -l 3 -g 4 -t 3 -o -e -v 24" >> 50M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 8 -q 19 -l 3 -g 4 -t 3 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_50M >> 50M_data.output
	echo "-f 8 -q 19 -l 3 -g 4 -t 4 -o -e -v 24" >> 50M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 8 -q 19 -l 3 -g 4 -t 4 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_50M >> 50M_data.output

data_gen500M:
	./streamdump -s 500000000 -f raw/streamdump_mmap_active_new_500M 12345 & \
	./firehose/generators/active/active -n 10000000 -r 500000 -a 1048576 127.0.0.1@12345

popcorn500M:
	echo "Scaling throughput for 500M dataset" > 500M_output.data
	echo 134217728 > /var/cgroups/popcorning/memory.limit_in_bytes
	echo "-f 256 -q 16 -l 4 -g 4 -t 1 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 1 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output
	echo "-f 256 -q 16 -l 4 -g 4 -t 2 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 2 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output
	echo "-f 256 -q 16 -l 4 -g 4 -t 4 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 4 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output
	echo "-f 256 -q 16 -l 4 -g 4 -t 8 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 8 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output
	echo "-f 256 -q 16 -l 4 -g 4 -t 16 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 16 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output
	echo "-f 256 -q 16 -l 4 -g 4 -t 32 -o -e -v 24" >> 500M_output.data
	cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 32 \
		-o -e -v 24 -i raw/streamdump_mmap_active_new_500M >> 500M_data.output


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
	rm -rf $(OBJDIR) $(LOGDIR) firehose core $(TARGETS)
