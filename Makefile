CC=g++
SRC=server.cpp
OBJ=server

all: $(OBJ)

proxy: $(SRC)
	$(CC) $< -o $@

clean:
	rm -rf $(OBJ)

.PHONY: all clean
