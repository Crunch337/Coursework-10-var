CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra
TARGET = store_sim.exe

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o results.csv

.PHONY: all run clean
