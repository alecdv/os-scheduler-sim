#CXXFLAGS += -g
DEBUG_FLAGS = -g -DDEBUG -std=gnu++11 #-Wall -Wextra#-O2
CXXFLAGS=$(DEBUG_FLAGS)

all: simulator

simulator: main.o simulation.o
	g++ $(CXXFLAGS) -o simulator main.o simulation.o

main.o: simulation.h
simulation.o: simulation.h