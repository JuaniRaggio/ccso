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

# Player strategies
STRATEGIES = naive bfs dfs minimax a_star
PLAYERS ?= $(STRATEGIES)

# Source discovery (main.c at module root + sources in src/ and utils/)
MASTER_SRCS = master/main.c $(wildcard master/src/*.c) $(wildcard master/utils/*.c)
PLAYER_SRCS = player/main.c $(wildcard player/src/*.c)
VIEW_SRCS   = view/main.c $(wildcard view/src/*.c)
COMMON_SRCS = $(wildcard common/src/*.c)

# Binaries
MASTER_BIN = $(BUILD_DIR)/master
VIEW_BIN   = $(BUILD_DIR)/view
PLAYER_BINS = $(addprefix $(BUILD_DIR)/player-,$(STRATEGIES))

# Map strategy name to -D flag (uppercase)
strategy_flag = $(shell echo $(1) | tr 'a-z' 'A-Z')

.PHONY: all players run clean compile_flags $(addprefix player-,$(STRATEGIES))

all: $(MASTER_BIN) players $(VIEW_BIN)

players: $(PLAYER_BINS)

# Individual player-<strategy> phony targets
$(addprefix player-,$(STRATEGIES)): player-%: $(BUILD_DIR)/player-%

# Pattern rule: build/player-<strategy>
$(BUILD_DIR)/player-%: $(PLAYER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -D$(call strategy_flag,$*) -I$(COMMON_INC) -Iplayer/include $^ -o $@ $(LDFLAGS)

$(MASTER_BIN): $(MASTER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Imaster/include -Imaster/utils $^ -o $@ $(LDFLAGS)

$(VIEW_BIN): $(VIEW_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Iview/include $^ -o $@ $(LDFLAGS)

# Build player paths from PLAYERS variable for run target
PLAYER_RUN_BINS = $(addprefix ./$(BUILD_DIR)/player-,$(PLAYERS))
PLAYER_RUN_TARGETS = $(addprefix $(BUILD_DIR)/player-,$(PLAYERS))

run: $(MASTER_BIN) $(PLAYER_RUN_TARGETS) $(VIEW_BIN)
	./$(MASTER_BIN) -v ./$(VIEW_BIN) -p $(PLAYER_RUN_BINS)

clean:
	rm -rf $(BUILD_DIR)

# Generate compile_flags.txt for clangd / IDEs
compile_flags:
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-Iutils" > master/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-DNAIVE" > player/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" > view/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-Iinclude" > common/compile_flags.txt
	@echo "compile_flags.txt generated for all modules"
