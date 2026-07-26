ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

TARGET := client.exe

all: $(TARGET)

$(TARGET): $(OBJS) BUILD_LIB
	$(GCC) $(OBJS) $(LIBS) -o $(TARGET)

BUILD_LIB:
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@ -MP -MMD

clean:
	rm -rf *.o *.d $(TARGET)

-include $(DEPS)