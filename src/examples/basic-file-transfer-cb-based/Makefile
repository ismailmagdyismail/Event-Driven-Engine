ROOT_PATH := ../../..
include $(ROOT_PATH)/Common.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

SERVER_TARGET := server_file_transfer.exe
CLIENT_TARGET := client_file_transfer.exe
COMPARATOR_TARGET:= comparator.exe
SERVER_MAIN := FileTransferServer
CLIENT_MAIN := FileTransferClient
COMPARATOR_MAIN:= comparator

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_MAIN).o BUILD_LIB
	$(GCC) $(SERVER_MAIN).o $(LIBS) -o $@

$(CLIENT_TARGET): $(CLIENT_MAIN).o BUILD_LIB
	$(GCC) $(CLIENT_MAIN).o $(LIBS) -o $@

BUILD_LIB:
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile

$(SERVER_MAIN).o: $(SERVER_MAIN).cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

$(CLIENT_MAIN).o: $(CLIENT_MAIN).cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf *.o *.d $(CLIENT_TARGET) $(SERVER_TARGET)
	$(MAKE) -C $(LIB_ROOT_PATH) -f Makefile clean

-include $(DEPS)