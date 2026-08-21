ROOT_PATH := ../../
HTTP_ROOT_PATH := ../

include $(ROOT_PATH)/Common.mk
include $(HTTP_ROOT_PATH)/Http.mk

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

SHARED_TARGET := lib$(HTTP_LIB_NAME).so
STATIC_TARGET := lib$(HTTP_LIB_NAME).a

all: $(SHARED_TARGET)

$(SHARED_TARGET): $(OBJS)
	$(GCC) $(SHARED_LIB_FLAGS) $(OBJS) -o $(SHARED_TARGET)

$(STATIC_TARGET): $(OBJS)
	ar rcs $(OBJS) -o $(STATIC_TARGET)

%.o: %.cpp
	$(GCC) $(CXX_FLAGS) $(INCLUDES) $(HTTP_INCLUDES) -c $< -o $@

clean:
	rm -rf *.o *.d $(STATIC_TARGET) $(SHARED_TARGET)

-include $(DEPS)