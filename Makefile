# Compiler
CC := clang

# Build type
BUILD ?= debug

CFLAGS := -std=c17 -Wall -Wextra -Wpedantic

ifeq ($(BUILD),release)
	CFLAGS += -O3 -DNDEBUG
	BUILD_DIR := build/release
else
	CFLAGS += -g3 -O0
	BUILD_DIR := build/debug
endif

AR := ar
ARFLAGS := rcs

LIB_DIR := libtpxl
CLI_DIR := cli

INCLUDES := \
    -I$(LIB_DIR)/include \
    -Ithird_party

LIB := $(BUILD_DIR)/libtpxl.a
CLI := $(BUILD_DIR)/tpxl

LIB_SRC := $(wildcard $(LIB_DIR)/src/*.c)
CLI_SRC := $(wildcard $(CLI_DIR)/src/*.c)

LIB_OBJ := $(patsubst $(LIB_DIR)/src/%.c,$(BUILD_DIR)/lib/%.o,$(LIB_SRC))
CLI_OBJ := $(patsubst $(CLI_DIR)/src/%.c,$(BUILD_DIR)/cli/%.o,$(CLI_SRC))

.PHONY: all lib cli clean debug release

all: lib cli

lib: $(LIB)

cli: $(CLI)

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

$(LIB): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

$(CLI): $(CLI_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/cli/%.o: $(CLI_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build