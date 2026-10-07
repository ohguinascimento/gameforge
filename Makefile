CXX ?= g++
CXXFLAGS = -std=c++20 -O2 -Wall -Iinclude
SRC = $(wildcard src/*.cpp)
TARGET = bin/gamec.exe

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf bin/*.exe bin/*.o

.PHONY: all clean
