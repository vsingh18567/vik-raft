

raft-server: src/main.cc src/*.h src/**/*.h
	g++ -std=c++17 -g -o raft-server src/main.cc

clean:
	rm -f raft-server