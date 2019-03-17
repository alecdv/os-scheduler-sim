#CXXFLAGS += -g
DEBUG_FLAGS = -g -Wall -Wextra -DDEBUG -std=gnu++11 #-O2
CXXFLAGS=$(DEBUG_FLAGS)

all: simulator

simulator: main.o simulation.o
	g++ $(CXXFLAGS) -o simulator main.o simulation.o

main.o: simulation.h
simulation.o: simulation.h