ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

TARGET := server.exe

all: $(TARGET)

$(TARGET): $(OBJS) BUILD_LIB
# 	$(GCC) $(OBJS) -L$(SOCKET_LIB_PATH)/ -l$(SOCKET_LIB_NAME) -Wl,-rpath,@executable_path/../lib/socket -o $(TARGET)
	$(GCC) $(OBJS) $(LIBS) -o $(TARGET)

BUILD_LIB:
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf *.o *.d $(TARGET)
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile clean