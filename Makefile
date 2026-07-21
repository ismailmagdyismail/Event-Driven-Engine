ROOT_PATH := .
include $(ROOT_PATH)/Common.mk

all: SERVER CLIENT

SERVER:
	$(MAKE) -C $(SERVER_PATH) -f Makefile

CLIENT:
	$(MAKE) -C $(CLIENT_PATH) -f Makefile

clean: CLEAN_SERVER CLEAN_CLIENT CLEAN_LIB

CLEAN_CLIENT:
	$(MAKE) -C $(CLIENT_PATH) clean -f Makefile

CLEAN_SERVER:
	$(MAKE) -C $(SERVER_PATH) clean -f Makefile

CLEAN_LIB:
	$(MAKE) -C $(SOCKET_LIB_PATH) clean -f Makefile
