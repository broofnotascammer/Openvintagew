# OpenVintage Phase 7 Rearchitecture - Root Master Makefile

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g \
	-Iapp/core_api \
	-Icore/hardware \
	-Icore/compatibility \
	-Icore/configuration \
	-Icore/boot \
	-Icore/deployment \
	-Icore/installer \
	-Icore/resolver \
	-Iplatforms/common \
	-Iplatforms/macos \
	-Iplatforms/darwin \
	-Isecurity \
	-Iintegrations/oclp \
	-Iintegrations/refind \
	-Iovir/common \
	-Iovir/cpu \
	-Iovir/gpu \
	-IOpenVintagePrebootSimulator/include \
	-IOpenVintagePrebootSimulator/platform/common \
	-IOpenVintagePrebootSimulator/platform/macos

LDFLAGS ?= -lpthread -lm -lpci

CLI_TARGET = bin/openvintage-cli
TEST_TARGET = bin/ov-test-suite

all: $(CLI_TARGET) $(TEST_TARGET)

$(CLI_TARGET): app/cli/main_cli.c app/core_api/ov_app_api.c \
               OpenVintagePrebootSimulator/src/ov_types.c \
               OpenVintagePrebootSimulator/src/ov_logger.c \
               OpenVintagePrebootSimulator/src/ov_memory.c \
               OpenVintagePrebootSimulator/src/ov_hardware.c \
               OpenVintagePrebootSimulator/src/ov_macos_compat.c \
               OpenVintagePrebootSimulator/src/ov_resolver.c \
               OpenVintagePrebootSimulator/src/ov_cpu_engine.c \
               OpenVintagePrebootSimulator/src/ov_gpu_engine.c \
               OpenVintagePrebootSimulator/src/ov_resource_manager.c \
               OpenVintagePrebootSimulator/src/ov_unified_cache.c \
               OpenVintagePrebootSimulator/src/ov_compatibility.c \
               OpenVintagePrebootSimulator/src/ov_benchmark.c \
               OpenVintagePrebootSimulator/src/ov_diagnostics.c \
               OpenVintagePrebootSimulator/src/ov_core.c \
               OpenVintagePrebootSimulator/src/ov_ui.c \
               OpenVintagePrebootSimulator/src/ov_security.c \
               OpenVintagePrebootSimulator/src/ov_perf_profile.c \
               OpenVintagePrebootSimulator/src/ov_boot_picker.c \
               OpenVintagePrebootSimulator/src/ov_deployment.c \
               OpenVintagePrebootSimulator/src/ov_efi_installer.c \
               OpenVintagePrebootSimulator/src/ov_oclp_adapter.c \
               OpenVintagePrebootSimulator/src/ov_refind_adapter.c \
               OpenVintagePrebootSimulator/platform/common/ov_platform.c \
               OpenVintagePrebootSimulator/platform/linux/ov_hardware_linux.c \
               OpenVintagePrebootSimulator/platform/macos/ov_hardware_macos.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@chmod +x $@
	@echo "OpenVintage CLI built successfully: $@"

$(TEST_TARGET): OpenVintagePrebootSimulator/tests/test_runner.c \
               app/core_api/ov_app_api.c \
               OpenVintagePrebootSimulator/src/ov_types.c \
               OpenVintagePrebootSimulator/src/ov_logger.c \
               OpenVintagePrebootSimulator/src/ov_memory.c \
               OpenVintagePrebootSimulator/src/ov_hardware.c \
               OpenVintagePrebootSimulator/src/ov_macos_compat.c \
               OpenVintagePrebootSimulator/src/ov_resolver.c \
               OpenVintagePrebootSimulator/src/ov_cpu_engine.c \
               OpenVintagePrebootSimulator/src/ov_gpu_engine.c \
               OpenVintagePrebootSimulator/src/ov_resource_manager.c \
               OpenVintagePrebootSimulator/src/ov_unified_cache.c \
               OpenVintagePrebootSimulator/src/ov_compatibility.c \
               OpenVintagePrebootSimulator/src/ov_benchmark.c \
               OpenVintagePrebootSimulator/src/ov_diagnostics.c \
               OpenVintagePrebootSimulator/src/ov_core.c \
               OpenVintagePrebootSimulator/src/ov_ui.c \
               OpenVintagePrebootSimulator/src/ov_security.c \
               OpenVintagePrebootSimulator/src/ov_perf_profile.c \
               OpenVintagePrebootSimulator/src/ov_boot_picker.c \
               OpenVintagePrebootSimulator/src/ov_deployment.c \
               OpenVintagePrebootSimulator/src/ov_efi_installer.c \
               OpenVintagePrebootSimulator/src/ov_oclp_adapter.c \
               OpenVintagePrebootSimulator/src/ov_refind_adapter.c \
               OpenVintagePrebootSimulator/platform/common/ov_platform.c \
               OpenVintagePrebootSimulator/platform/linux/ov_hardware_linux.c \
               OpenVintagePrebootSimulator/platform/macos/ov_hardware_macos.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@chmod +x $@
	@echo "OpenVintage Test Suite built successfully: $@"

cli: $(CLI_TARGET)
	@chmod +x $(CLI_TARGET)

test: $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	./$(TEST_TARGET)

app: $(CLI_TARGET)
	./scripts/package_app.sh

clean:
	rm -rf bin/ build/ OpenVintagePrebootSimulator/bin OpenVintagePrebootSimulator/build OpenVintage.app

.PHONY: all cli test clean app
