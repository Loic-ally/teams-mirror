##
## EPITECH PROJECT, 2026
## My Teams
## Makefile
##

BUILD_DIR   = build
NPROC       = $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

all:
	@if [ -f $(BUILD_DIR)/CMakeCache.txt ] && ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=$(CURDIR)" $(BUILD_DIR)/CMakeCache.txt; then \
		echo "Resetting incompatible CMake cache in $(BUILD_DIR)"; \
		rm -rf $(BUILD_DIR); \
	fi
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel $(NPROC)

clean:
	@if [ -d $(BUILD_DIR) ]; then cmake --build $(BUILD_DIR) --target clean; fi
	find . -type f -name "*.gcda" -delete
	find . -type f -name "*.gcno" -delete
	find . -type f -name "*.gcov" -delete

fclean:
	rm -rf $(BUILD_DIR)
	rm -rf ./myteams_server ./myteams_cli
	find . -type f -name "*.gcda" -delete
	find . -type f -name "*.gcno" -delete
	find . -type f -name "*.gcov" -delete

re: fclean all
