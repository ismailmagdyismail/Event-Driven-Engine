ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

TARGET := server.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(GCC) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf *.o *.d $(TARGET)
