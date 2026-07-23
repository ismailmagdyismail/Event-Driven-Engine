ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

TARGET := client.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(GCC) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@ -MP -MMD

clean:
	rm -rf *.o *.d $(TARGET)

-include $(DEPS)