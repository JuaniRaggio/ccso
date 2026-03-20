CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
  LDFLAGS += -lrt
endif

# Directories
BUILD_DIR = build
COMMON_INC = common/include

# Source discovery
MASTER_SRCS = $(wildcard master/src/*.c) $(wildcard master/utils/*.c)
PLAYER_SRCS = $(wildcard player/src/*.c)
VIEW_SRCS   = $(wildcard view/src/*.c)
COMMON_SRCS = $(wildcard common/src/*.c)

# Binaries
MASTER_BIN = $(BUILD_DIR)/master
PLAYER_BIN = $(BUILD_DIR)/player
VIEW_BIN   = $(BUILD_DIR)/view

.PHONY: all run clean compile_flags

all: $(MASTER_BIN) $(PLAYER_BIN) $(VIEW_BIN)

$(MASTER_BIN): $(MASTER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Imaster/include -Imaster/utils $^ -o $@ $(LDFLAGS)

$(PLAYER_BIN): $(PLAYER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Iplayer/include $^ -o $@ $(LDFLAGS)

$(VIEW_BIN): $(VIEW_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Iview/include $^ -o $@ $(LDFLAGS)

# Run all 3 processes simultaneously
run: all
	@echo "Starting master, player, and view..."
	@./$(MASTER_BIN) & \
	 ./$(PLAYER_BIN) & \
	 ./$(VIEW_BIN) & \
	 wait

clean:
	rm -rf $(BUILD_DIR)

# Generate compile_flags.txt for clangd / IDEs
compile_flags:
	@printf -- "-Wall\n-g\n-I../common/include\n-Iinclude\n-Iutils\n" > master/compile_flags.txt
	@printf -- "-Wall\n-g\n-I../common/include\n-Iinclude\n" > player/compile_flags.txt
	@printf -- "-Wall\n-g\n-I../common/include\n-Iinclude\n" > view/compile_flags.txt
	@printf -- "-Wall\n-g\n-Iinclude\n" > common/compile_flags.txt
	@echo "compile_flags.txt generated for all modules"
