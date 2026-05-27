.PHONY: all configure build run clean

BUILD_DIR ?= build

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/main

clean:
	rm -rf $(BUILD_DIR)
