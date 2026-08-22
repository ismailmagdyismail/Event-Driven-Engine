ROOT_PATH := ..
HTTP_ROOT_PATH := .
include $(ROOT_PATH)/Common.mk
include $(HTTP_ROOT_PATH)/Http.mk

all: BUILD_HTTP_LIB

BUILD_HTTP_LIB:
	$(MAKE) -C $(HTTP_LIB_ROOT_PATH) -f Makefile

clean:
	$(MAKE) -C $(HTTP_LIB_ROOT_PATH) -f Makefile clean