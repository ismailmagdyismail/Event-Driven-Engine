ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

TARGET := server.exe

all: $(TARGET)

$(TARGET): $(OBJS) SOCKET_LIB
# 	$(GCC) $(OBJS) -L$(SOCKET_LIB_PATH)/ -l$(SOCKET_LIB_NAME) -Wl,-rpath,@executable_path/../lib/socket -o $(TARGET)
	$(GCC) $(OBJS) -L$(SOCKET_LIB_PATH)/ -l$(SOCKET_LIB_NAME) -o $(TARGET)

SOCKET_LIB:
	$(MAKE) -C $(SOCKET_LIB_PATH) -f Makefile

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf *.o *.d $(TARGET)
