export CC		:= clang
export CXX		:= clang++

N_PROCS			:= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

BUILD_DIR		:= build
BIN_DIR			:= $(BUILD_DIR)/bin
GENERATOR		:= Ninja
CMAKE_FLAGS		:= -G $(GENERATOR) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

ifeq ($(OS),Windows_NT)
	EXECUTABLE	:= hadronc.exe
	TEST_EXE	:= hadron_tests.exe
else
	EXECUTABLE	:= hadronc
	TEST_EXE	:= hadron_tests
	GENERATOR	:= $(shell which ninja > /dev/null 2>&1 && echo "Ninja" || echo "Unix Makefiles")
endif

GREEN			:= $(shell printf "\033[32m")
BLUE			:= $(shell printf "\033[34m")
RESET			:= $(shell printf "\033[0m")

all: debug

link-compdb:
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		ln -sf $(BUILD_DIR)/compile_commands.json .; \
	fi

configure-debug:
	@echo "$(BLUE)Configuring Debug Build ($(GENERATOR))...$(RESET)"
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
	@$(MAKE) link-compdb

configure-release:
	@echo "$(BLUE)Configuring Release Build ($(GENERATOR))...$(RESET)"
	@cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF -DBUILD_TESTS=ON
	@$(MAKE) link-compdb

debug: configure-debug
	@echo "$(GREEN)Building Debug (Jobs: $(N_PROCS))...$(RESET)"
	@cmake --build $(BUILD_DIR) --parallel $(N_PROCS)

release: configure-release
	@echo "$(GREEN)Building Release (Jobs: $(N_PROCS))...$(RESET)"
	@cmake --build $(BUILD_DIR) --parallel $(N_PROCS)

test: debug
	@echo "$(GREEN)Running Tests...$(RESET)"
	@cd $(BUILD_DIR) && ctest --output-on-failure -j $(N_PROCS)

run: debug
	@echo "$(GREEN)Running Hadron Compiler...$(RESET)"
	@$(BUILD_DIR)/$(EXECUTABLE)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f compile_commands.json
	@echo "$(BLUE)Build directory cleaned.$(RESET)"

fmt:
	@echo "$(BLUE)Formatting code...$(RESET)"
	@find src include test -name '*.cpp' -o -name '*.hpp' -o -name '*.c' -o -name '*.h' | xargs clang-format -i
	@echo "$(GREEN)Code formatted.$(RESET)"

help:
	@printf "$(BLUE)Hadron Compiler Build System$(RESET)\n"
	@printf "==============================\n"
	@printf "Available commands:\n"
	@printf "%3s make debug%3s: Build with symbols and sanitizers (default)\n"
	@printf "%3s make release : Build optimized for release\n"
	@printf "%3s make test%4s: Run GTest suite\n"
	@printf "%3s make run%5s: Run Hadron compiler\n"
	@printf "%3s make clean%3s: Clean build artefacts\n"
	@printf "%3s make fmt%5s: Format code using clang-format"

.PHONY: all link-compdb configure-debug configure-release debug release clean test run fmt help
