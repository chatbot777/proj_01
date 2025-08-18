CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

SRC_DIR := src
SRC := $(SRC_DIR)/main.cpp
TARGET := main

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET)
