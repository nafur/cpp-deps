CXX=g++ -g -std=c++17

.PHONY: all clean
all: cpp-deps
clean:
	rm -f *.o cpp-deps

commands.o: commands.cpp commands.h
	${CXX} -c $< -o $@

graph.o: graph.cpp graph.h
	${CXX} -c $< -o $@

main.o: main.cpp commands.h graph.h
	${CXX} -c $< -o $@

cpp-deps: main.o commands.o graph.o
	g++ -g -std=c++17 $^ -lstdc++fs -lboost_program_options -lboost_system -lpthread -o $@
