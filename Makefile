CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g -Icore/boot -Icore/installer
CLI_TARGET = bin/openvintage-cli
TEST_TARGET = bin/test_boot_handoff

.PHONY: all test app clean usb

all: $(CLI_TARGET) $(TEST_TARGET)

$(CLI_TARGET): core/boot/ov_boot_picker.c core/installer/ov_efi_installer.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -DCLI_MAIN $^ -o $@
	@echo "OpenVintage CLI built: $@"

$(TEST_TARGET): tests/test_boot_handoff.c core/boot/ov_boot_picker.c core/installer/ov_efi_installer.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@
	@echo "OpenVintage Test Suite built: $@"

test: $(TEST_TARGET)
	./$(TEST_TARGET)
	npm test

app:
	@bash ./scripts/package_app.sh

usb:
	@bash ./scripts/package_usb.sh /tmp/openvintage_usb

clean:
	rm -rf bin/ build/ dist/ OpenVintage.app

