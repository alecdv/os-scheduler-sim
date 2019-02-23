simulator: main.o simulation.o
	g++ -o simulator main.o simulation.o

main.o: simulation.h
simulation.o: simulation.h