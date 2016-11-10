CXX = g++ -std=c++11

#CXXFLAGS = -Wall -g -I. -pthread -Wno-unused-result -Wno-strict-aliasing
CXXFLAGS = -Wall -Ofast -m64 -I. -Wno-unused-result -Wno-strict-aliasing
#CXXFLAGS = -Wall -Ofast -m64 -I. -Wno-unused-result -Wno-strict-aliasing -DLOG_WAIT_TIME -DLOG_CLUSTER_LENGTH

LDFLAGS = -lpthread -lssl -lcrypto -lboost_system -lboost_thread

TARGET_MAIN	= main
MAIN_SRC = main.cc threadsafe-gqf/gqf.c

$(TARGET_MAIN): $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) $(MAIN_SRC) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET_MAIN) *.o core
