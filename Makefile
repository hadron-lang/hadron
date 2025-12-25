export CC		:= clang
export CXX		:= clang++

BUILD_DIR		:= build
BIN_DIR			:= $(BUILD_DIR)/bin
GENERATOR		:= Ninja
CMAKE_FLAGS		:= -G $(GENERATOR)

ifeq ($(OS),Windows_NT)
	EXECUTABLE	:= hadronc.exe
	TEST_EXE	:= hadron_tests.exe
else
	EXECUTABLE	:= hadronc
	TEST_EXE	:= hadron_tests
	GENERATOR	:= $(shell which ninja > /dev/null 2>&1 && echo "Ninja" || echo "Unix Makefiles")
endif

GREEN			:= \033[32m
RESET			:= \033[0m

all: debug

configure-debug:
	@echo "$(GREEN)Configuring Debug Build...$(RESET)"
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON

configure-release:
	@echo "$(GREEN)Configuring Release Build...$(RESET)"
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF -DBUILD_TESTS=ON

debug: configure-debug
	@echo "$(GREEN)Building Debug...$(RESET)"
	@cmake --build $(BUILD_DIR)

release: configure-release
	@echo "$(GREEN)Building Release...$(RESET)"
	@cmake --build $(BUILD_DIR)

test: debug
	@echo "$(GREEN)Running Tests...$(RESET)"
	@cd $(BUILD_DIR) && ctest --output-on-failure

run: debug
	@echo "$(GREEN)Running Hadron Compiler...$(RESET)"
	@$(BUILD_DIR)/$(EXECUTABLE)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build directory removed."

fmt:
	@find src include tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
	@echo "Code formatted."

help:
	@printf "===== Available Commands =====\n"
	@printf "%3s make debug%3s: Build with symbols and sanitizers (default)\n"
	@printf "%3s make release : Build optimized for release\n"
	@printf "%3s make test%4s: Run GTest tests suite\n"
	@printf "%3s make run%5s: Run Hadron compiler\n"
	@printf "%3s make clean%3s: Clean build artefacts\n"


.PHONY: all configure-debug configure-release debug release clean test run fmt help
