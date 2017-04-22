CC		= g++
CFLAGS		= -O3 -Wall -std=c++11 
LDFLAGS		= -lprotobuf

SRC		= main.cpp commands.pb.cc

all		: clear_screen pas-sim

pas-sim		: main.o commands.pb.o
		$(CC) $^ -o $@ $(LDFLAGS)

%.o		: %.cc 
		$(CC) -c $(CFLAGS) $< -o $@

%.o		: %.cpp 
		$(CC) -c $(CFLAGS) $< -o $@

.PHONY		: clear_screen

clear_screen	:
		clear

clean		:
		rm -f *.o pas-sim

