ROOT_PATH := .
include $(ROOT_PATH)/Common.mk

SRC_PATH := $(ROOT_PATH)/src
SERVER_PATH := $(SRC_PATH)/server
CLIENT_PATH := $(SRC_PATH)/client

all: SERVER CLIENT

SERVER:
	$(MAKE) -C $(SERVER_PATH) -f Makefile

CLIENT:
	$(MAKE) -C $(CLIENT_PATH) -f Makefile

clean: CLEAN_SERVER CLEAN_CLIENT

CLEAN_CLIENT:
	$(MAKE) -C $(CLIENT_PATH) clean -f Makefile

CLEAN_SERVER:
	$(MAKE) -C $(SERVER_PATH) clean -f Makefile
