# OpenVintage Pre-Boot Architecture Simulator (Phases 1-5) Makefile

CC = gcc
CFLAGS ?= -Wall -Wextra -O2 -g -Iinclude
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
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/openvintage-preboot-simulator

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/ov_logger.c \
       $(SRC_DIR)/ov_memory.c \
       $(SRC_DIR)/ov_hardware.c \
       $(SRC_DIR)/ov_resolver.c \
       $(SRC_DIR)/ov_cpu_engine.c \
       $(SRC_DIR)/ov_gpu_engine.c \
       $(SRC_DIR)/ov_resource_manager.c \
       $(SRC_DIR)/ov_unified_cache.c \
       $(SRC_DIR)/ov_compatibility.c \
       $(SRC_DIR)/ov_benchmark.c \
       $(SRC_DIR)/ov_diagnostics.c \
       $(SRC_DIR)/ov_core.c \
       $(SRC_DIR)/ov_ui.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build successful: $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) openvintage_preboot.log openvintage_report.txt openvintage_diagnostics.*

test: $(TARGET)
	@echo "Running OpenVintage Pre-Boot Simulator Self-Test in CLI mode..."
	$(TARGET) --cli --bench --report test_report.txt --html test_report.html --json test_report.json
	@echo "All Pre-Boot Simulator Self-Tests Passed!"

run: $(TARGET)
	$(TARGET)
