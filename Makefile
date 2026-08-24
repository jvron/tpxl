CC := clang
AR := ar
ARFLAGS := rcs

BUILD ?= debug

CFLAGS := -std=c17 -Wall -Wextra -Wpedantic

ifeq ($(BUILD),release)

    CFLAGS += -O3 -DNDEBUG
    BUILD_DIR := build/release

else ifeq ($(BUILD),asan)

    CFLAGS += -O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
    BUILD_DIR := build/asan

else ifeq ($(BUILD),tsan)

    CFLAGS += -O1 -g3 -fsanitize=thread -fno-omit-frame-pointer
    BUILD_DIR := build/tsan

else

    CFLAGS += -g3 -O0
    BUILD_DIR := build/debug

endif


LIB_DIR := libtpxl
CLI_DIR := cli
BASE64_DIR := third_party/base64

BASE64_CFLAGS := \
    -mavx2 \
    -mssse3 \
    -msse4.1 \
    -msse4.2 \
    -mavx

MINIAUDIO_SRC := third_party/miniaudio/miniaudio.c
MINIAUDIO_OBJ := $(BUILD_DIR)/lib/miniaudio.o

LDLIBS := \
    -lm \
    -ldl \
    -lpthread \
    -lavformat \
    -lavcodec \
    -lavutil \
    -lswscale \
    -lswresample \
    -lgif

INCLUDES := \
    -I$(LIB_DIR)/include \
    -I$(LIB_DIR)/src \
    -I$(BASE64_DIR)/include \
    -I$(CLI_DIR) \
    -Ithird_party


LIB := $(BUILD_DIR)/libtpxl.a
CLI := $(BUILD_DIR)/tpxl

BASE64_OBJ := $(BASE64_DIR)/lib/libbase64.o

LIB_SRC := $(shell find $(LIB_DIR)/src -name "*.c")
CLI_SRC := $(wildcard $(CLI_DIR)/*.c)

LIB_OBJ := $(patsubst \
    $(LIB_DIR)/src/%.c, \
    $(BUILD_DIR)/lib/%.o, \
    $(LIB_SRC))

CLI_OBJ := $(patsubst \
    $(CLI_DIR)/%.c, \
    $(BUILD_DIR)/cli/%.o, \
    $(CLI_SRC))


.PHONY: all lib cli debug release asan tsan clean


all: lib cli

lib: $(LIB)

cli: $(CLI)


debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

asan:
	$(MAKE) BUILD=asan

tsan:
	$(MAKE) BUILD=tsan


$(LIB): $(LIB_OBJ) $(BASE64_OBJ) $(MINIAUDIO_OBJ)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^


$(CLI): $(CLI_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@


$(BASE64_OBJ):
	$(MAKE) -C $(BASE64_DIR) \
		lib/libbase64.o \
		CC=$(CC) \
		AVX2_CFLAGS=-mavx2 \
		SSSE3_CFLAGS=-mssse3 \
		SSE41_CFLAGS=-msse4.1 \
		SSE42_CFLAGS=-msse4.2 \
		AVX_CFLAGS=-mavx


$(MINIAUDIO_OBJ): $(MINIAUDIO_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


$(BUILD_DIR)/cli/%.o: $(CLI_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


clean:
	rm -rf build