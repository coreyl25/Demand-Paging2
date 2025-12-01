CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Targets
all: tlb_pagetable clock_replacement

tlb_pagetable: $(SRC_DIR)/tlb_pagetable.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/tlb_pagetable $<

clock_replacement: $(SRC_DIR)/clock_replacement.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/clock_replacement $<

clean:
	rm -rf $(BUILD_DIR)

test: all
	@echo "Running tests..."
	@chmod +x $(TEST_DIR)/test_all.sh
	@$(TEST_DIR)/test_all.sh

.PHONY: all clean test