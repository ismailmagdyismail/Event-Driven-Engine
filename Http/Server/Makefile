ROOT_PATH := ../../
HTTP_ROOT_PATH := ../
include $(ROOT_PATH)/Common.mk
include $(HTTP_ROOT_PATH)/http.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

TARGET := http_server.exe

all: $(TARGET)

$(TARGET): $(OBJS) BUILD_ASYNC_LIB BUILD_HTTP_LIB
	$(GCC) $(OBJS) $(LIBS) $(HTTP_LIBS) -o $(TARGET)

BUILD_ASYNC_LIB:
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile

BUILD_HTTP_LIB:
	$(MAKE) -C $(HTTP_ROOT_PATH) -f Makefile

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) $(HTTP_INCLUDES) -c $< -o $@

clean:
	rm -rf *.o *.d $(TARGET)
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile clean
	$(MAKE) -C $(HTTP_LIB_ROOT_PATH) -f Makefile clean

-include $(DEPS)