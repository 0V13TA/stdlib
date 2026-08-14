CC := clang
CFLAGS := -std=c99 -Wall -Wextra -pedantic -g
MUNIT_CFLAGS := $(CFLAGS) -Wno-c11-extensions
BUILD_DIR := bin

TEST_SRCS := $(wildcard *_main.c)
LIB_SRCS := $(filter-out munit.c $(TEST_SRCS),$(wildcard *.c))
HEADERS := $(wildcard *.h)

TEST_BINS := $(patsubst %_main.c,$(BUILD_DIR)/%_tests,$(TEST_SRCS))
LIB_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
MUNIT_OBJ := $(BUILD_DIR)/munit.o
TEST_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))
OBJS := $(LIB_OBJS) $(MUNIT_OBJ) $(TEST_OBJS)

.PHONY: all test clean
.SECONDARY: $(OBJS)

all: $(TEST_BINS)

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do \
		echo "==> $$test_bin"; \
		./$$test_bin; \
	done

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/munit.o: munit.c munit.h | $(BUILD_DIR)
	$(CC) $(MUNIT_CFLAGS) -c munit.c -o $@

$(BUILD_DIR)/%_tests: $(BUILD_DIR)/%_main.o $(LIB_OBJS) $(MUNIT_OBJ)
	$(CC) $^ -o $@

clean:
	rm -f $(BUILD_DIR)/*_tests $(OBJS)
