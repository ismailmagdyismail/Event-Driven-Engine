ROOT_PATH := ../..
include $(ROOT_PATH)/Common.mk

all: BUILD_LIB

BUILD_LIB: BUILD_ENGINE BUILD_HELPERS BUILD_SOCKETS

BUILD_SOCKETS:
	$(MAKE) -C $(SOCKET_LIB_PATH) -f Makefile

BUILD_ENGINE:
	$(MAKE) -C $(ASYNC_ENGINE_LIB_PATH) -f Makefile

BUILD_HELPERS:
	$(MAKE) -C $(HELPERS_LIB_PATH) -f Makefile
	
clean:
	$(MAKE) -C $(HELPERS_LIB_PATH) -f Makefile clean
	$(MAKE) -C $(ASYNC_ENGINE_LIB_PATH) -f Makefile clean
	$(MAKE) -C $(SOCKET_LIB_PATH) -f Makefile clean
