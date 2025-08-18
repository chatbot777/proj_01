CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

SRC_DIR := src
INC_DIR := inc
OBJ_DIR := obj
LOG_DIR := log

CPPFLAGS := -I$(INC_DIR)
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TARGET := main

.RECIPEPREFIX := >

$(TARGET): $(OBJS)
>$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
>$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
>mkdir -p $(OBJ_DIR)

.PHONY: clean
clean:
>rm -rf $(TARGET) $(OBJ_DIR) $(LOG_DIR)
