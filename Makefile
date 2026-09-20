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
	bash ./scripts/package_app.sh

clean:
	rm -rf bin/ build/ OpenVintagePrebootSimulator/bin OpenVintagePrebootSimulator/build OpenVintage.app

.PHONY: all cli test clean app