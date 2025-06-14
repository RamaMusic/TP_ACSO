# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread -g

# Build targets
TARGET = threadpool

# Default target (you can comment this out if not needed)
all: custom

# === Basic test target ===
basic:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc test_basic.cc

# === Advanced test target ===
advanced:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc test_advanced.cc

# === Main file target ===
main:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc main.cc

# === Custom test target ===
custom:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc test_custom.cc

# Clean target
clean:
	rm -f $(TARGET)
