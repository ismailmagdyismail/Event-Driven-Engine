SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

TARGET := server.exe
GCC := g++
CXX_FLAGS := -std=c++17

all: $(TARGET)

$(TARGET): $(OBJS)
	$(GCC) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf *.o *.d $(TARGET)
