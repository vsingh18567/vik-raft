

TARGET = raft-server

raft-server: src/main.cc src/**/*.h
	g++ -std=c++17 -o $(TARGET) src/main.cc

clean:
	rm -f $(TARGET)