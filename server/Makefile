CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = server
SOURCES = server.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
