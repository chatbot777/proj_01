# C++ compiler and standard build options
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Project structure
SRC_DIR := src
INC_DIR := inc
OBJ_DIR := obj
LOG_DIR := log

# Include paths and file lists
CPPFLAGS := -I$(INC_DIR)
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
TARGET := main

# Use '>' as the command prefix for recipes
.RECIPEPREFIX := >

# Link the final executable from object files
$(TARGET): $(OBJS)
>$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
>$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Ensure that the object directory exists
$(OBJ_DIR):
>mkdir -p $(OBJ_DIR)

# Remove build artifacts
.PHONY: clean
clean:
>rm -rf $(TARGET) $(OBJ_DIR) $(LOG_DIR)
