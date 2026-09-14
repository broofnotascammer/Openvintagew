# OpenVintage Pre-Boot Architecture Simulator (Phases 1-5) Makefile

CC = gcc
CFLAGS ?= -Wall -Wextra -O2 -g -Iinclude -Iplatform/common -Iplatform/macos
LDFLAGS ?= -lpthread -lm

# Detect libpci
PCI_CFLAGS := $(shell pkg-config --cflags libpci 2>/dev/null)
PCI_LIBS := $(shell pkg-config --libs libpci 2>/dev/null || echo "-lpci")

# Detect GTK4
GTK_CFLAGS := $(shell pkg-config --cflags gtk4 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk4 2>/dev/null)

ifneq ($(GTK_CFLAGS),)
CFLAGS += $(GTK_CFLAGS) $(PCI_CFLAGS) -DOV_ENABLE_GTK
LDFLAGS += $(GTK_LIBS) $(PCI_LIBS)
else
CFLAGS += $(PCI_CFLAGS)
LDFLAGS += $(PCI_LIBS)
endif

SRC_DIR = src
INC_DIR = include
PLATFORM_DIR = platform
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/openvintage-preboot-simulator
TEST_TARGET = $(BIN_DIR)/test_runner
ASAN_TEST_TARGET = $(BIN_DIR)/test_runner_asan

COMMON_SRCS = $(SRC_DIR)/ov_types.c \
              $(SRC_DIR)/ov_logger.c \
              $(SRC_DIR)/ov_memory.c \
              $(SRC_DIR)/ov_hardware.c \
              $(SRC_DIR)/ov_macos_compat.c \
              $(SRC_DIR)/ov_resolver.c \
              $(SRC_DIR)/ov_cpu_engine.c \
              $(SRC_DIR)/ov_gpu_engine.c \
              $(SRC_DIR)/ov_resource_manager.c \
              $(SRC_DIR)/ov_unified_cache.c \
              $(SRC_DIR)/ov_compatibility.c \
              $(SRC_DIR)/ov_benchmark.c \
              $(SRC_DIR)/ov_diagnostics.c \
              $(SRC_DIR)/ov_core.c \
              $(SRC_DIR)/ov_ui.c \
              $(SRC_DIR)/ov_security.c \
              $(SRC_DIR)/ov_perf_profile.c \
              $(SRC_DIR)/ov_boot_picker.c \
              $(SRC_DIR)/ov_deployment.c \
              $(SRC_DIR)/ov_oclp_adapter.c \
              $(SRC_DIR)/ov_refind_adapter.c \
              $(PLATFORM_DIR)/common/ov_platform.c \
              $(PLATFORM_DIR)/linux/ov_hardware_linux.c \
              $(PLATFORM_DIR)/macos/ov_hardware_macos.c

APP_SRCS = $(SRC_DIR)/main.c $(COMMON_SRCS)
TEST_SRCS = tests/test_runner.c $(COMMON_SRCS)

OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/ov_types.o \
       $(BUILD_DIR)/ov_logger.o \
       $(BUILD_DIR)/ov_memory.o \
       $(BUILD_DIR)/ov_hardware.o \
       $(BUILD_DIR)/ov_macos_compat.o \
       $(BUILD_DIR)/ov_resolver.o \
       $(BUILD_DIR)/ov_cpu_engine.o \
       $(BUILD_DIR)/ov_gpu_engine.o \
       $(BUILD_DIR)/ov_resource_manager.o \
       $(BUILD_DIR)/ov_unified_cache.o \
       $(BUILD_DIR)/ov_compatibility.o \
       $(BUILD_DIR)/ov_benchmark.o \
       $(BUILD_DIR)/ov_diagnostics.o \
       $(BUILD_DIR)/ov_core.o \
       $(BUILD_DIR)/ov_ui.o \
       $(BUILD_DIR)/ov_security.o \
       $(BUILD_DIR)/ov_perf_profile.o \
       $(BUILD_DIR)/ov_boot_picker.o \
       $(BUILD_DIR)/ov_deployment.o \
       $(BUILD_DIR)/ov_oclp_adapter.o \
       $(BUILD_DIR)/ov_refind_adapter.o \
       $(BUILD_DIR)/platform/common/ov_platform.o \
       $(BUILD_DIR)/platform/linux/ov_hardware_linux.o \
       $(BUILD_DIR)/platform/macos/ov_hardware_macos.o

COMMON_OBJS = $(filter-out $(BUILD_DIR)/main.o,$(OBJS))
TEST_OBJS = $(BUILD_DIR)/test_runner.o $(COMMON_OBJS)

.PHONY: all clean test test-asan test-valgrind run macos macos-intel macos-arm64 macos-universal

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build successful: $@"

$(TEST_TARGET): $(TEST_OBJS) | $(BIN_DIR)
	$(CC) $(TEST_OBJS) -o $@ $(LDFLAGS)
	@echo "Test suite built: $@"

$(BUILD_DIR)/test_runner.o: tests/test_runner.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/common/%.o: $(PLATFORM_DIR)/common/%.c | $(BUILD_DIR)/platform/common
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/linux/%.o: $(PLATFORM_DIR)/linux/%.c | $(BUILD_DIR)/platform/linux
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/macos/%.o: $(PLATFORM_DIR)/macos/%.c | $(BUILD_DIR)/platform/macos
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/platform/common:
	mkdir -p $(BUILD_DIR)/platform/common

$(BUILD_DIR)/platform/linux:
	mkdir -p $(BUILD_DIR)/platform/linux

$(BUILD_DIR)/platform/macos:
	mkdir -p $(BUILD_DIR)/platform/macos

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

macos:
	@$(MAKE) -C platform/macos -f Makefile.macos macos

macos-intel:
	@$(MAKE) -C platform/macos -f Makefile.macos macos-intel

macos-arm64:
	@$(MAKE) -C platform/macos -f Makefile.macos macos-arm64

macos-universal:
	@$(MAKE) -C platform/macos -f Makefile.macos macos-universal

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) openvintage_preboot.log openvintage_report.txt openvintage_diagnostics.* test_report.* /tmp/ov_test_report.*

test: $(TEST_TARGET) $(TARGET)
	@echo "=== Running OpenVintage Unit & Subsystem Test Suite ==="
	$(TEST_TARGET)
	@echo "=== Running OpenVintage CLI Hardware & macOS Compatibility Verification ==="
	$(TARGET) --cli --mac MBP9,1 --compat-matrix --target-os monterey --bench --report test_report.txt --html test_report.html --json test_report.json
	@echo "=== All Tests and Architecture Validations Succeeded! ==="

test-asan: clean
	@echo "=== Building and Running Test Suite with AddressSanitizer & UBSan ==="
	mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_runner.c $(COMMON_SRCS) -o $(ASAN_TEST_TARGET) $(LDFLAGS) -fsanitize=address,undefined
	$(ASAN_TEST_TARGET)
	@echo "=== ASan & UBSan Tests Passed with Zero Leaks/Errors! ==="

test-valgrind: $(TEST_TARGET)
	@which valgrind >/dev/null 2>&1 || (echo "valgrind not installed, skipping" && exit 0)
	valgrind --leak-check=full --error-exitcode=1 $(TEST_TARGET)

run: $(TARGET)
	$(TARGET)
