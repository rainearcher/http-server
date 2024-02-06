CC=g++
SRC=server.cpp smartSocket.cpp
INCLUDES=smartSocket.h
EXE=server

all: $(EXE) 
	
$(EXE): $(SRC)
	$(CC) -o $(EXE) $(SRC)

proxy: $(SRC)
	$(CC) $< -o $@

clean:
	rm -rf $(EXE)

.PHONY: all clean
