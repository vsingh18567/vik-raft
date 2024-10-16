

raft-server: src/main.cc src/**.h
	g++ -std=c++17 -o raft-server src/main.cc

clean:
	rm -f vikraft