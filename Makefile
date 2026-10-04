CXXFLAGS = -std=c++17 -Wall -Wextra -O2

ifeq ($(OS),Windows_NT)    //if running the make file on windows.
RM = cmd /C del /Q /F
RUN = circuit_simulator.exe
else						//if running the make file on Ubantu(Linux).
RM = rm -f
RUN = ./circuit_simulator
endif

COMMON_OBJS = circuit.o components.o simulator.o analysis.o csv_io.o

all: circuit_simulator tests csv_tests

main.o: main.cpp
	g++ $(CXXFLAGS) -c main.cpp -o main.o

circuit.o: circuit.cpp
	g++ $(CXXFLAGS) -c circuit.cpp -o circuit.o

components.o: components.cpp
	g++ $(CXXFLAGS) -c components.cpp -o components.o

simulator.o: simulator.cpp
	g++ $(CXXFLAGS) -c simulator.cpp -o simulator.o

analysis.o: analysis.cpp
	g++ $(CXXFLAGS) -c analysis.cpp -o analysis.o

csv_io.o: csv_io.cpp
	g++ $(CXXFLAGS) -c csv_io.cpp -o csv_io.o

tests.o: tests.cpp
	g++ $(CXXFLAGS) -c tests.cpp -o tests.o

csv_tests.o: csv_tests.cpp
	g++ $(CXXFLAGS) -c csv_tests.cpp -o csv_tests.o

circuit_simulator: main.o $(COMMON_OBJS)
	g++ $(CXXFLAGS) -o circuit_simulator main.o $(COMMON_OBJS)

tests: tests.o $(COMMON_OBJS)
	g++ $(CXXFLAGS) -o tests tests.o $(COMMON_OBJS)

csv_tests: csv_tests.o $(COMMON_OBJS)
	g++ $(CXXFLAGS) -o csv_tests csv_tests.o $(COMMON_OBJS)

run: circuit_simulator
	$(RUN)

clean:
	$(RM) *.o circuit_simulator tests csv_tests circuit_simulator.exe tests.exe csv_tests.exe

.PHONY: all clean run
