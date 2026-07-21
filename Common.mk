GCC := g++
CXX_FLAGS := -std=c++17

SRC_PATH := $(ROOT_PATH)/src
SERVER_PATH := $(SRC_PATH)/server
CLIENT_PATH := $(SRC_PATH)/client
SOCKET_LIB_PATH := $(SRC_PATH)/lib/socket

SOCKET_LIB_NAME := async_socket_io


INCLUDES := -I$(SOCKET_LIB_PATH)/