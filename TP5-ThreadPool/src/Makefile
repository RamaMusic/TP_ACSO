# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread -g

# Build targets
TARGET = threadpool

# Default target (you can comment this out if not needed)
all: advanced

# === Basic test target ===
basic:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc tptest_extra_info.cc

# === Advanced test target ===
advanced:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc tpcustomtest_info.cc

# === Custom mega test target ===
custom:
	$(CXX) $(CXXFLAGS) -o $(TARGET) thread-pool.cc Semaphore.cc test_rama.cc

# Clean target
clean:
	rm -f $(TARGET)
