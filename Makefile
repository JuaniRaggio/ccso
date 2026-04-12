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
STRATEGIES = naive greedy greedy_lookahead flood greedy_flood
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

.PHONY: all players run clean compile_flags test memcheck $(addprefix player-,$(STRATEGIES))

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
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Iview/include $^ -o $@ $(LDFLAGS) -lncurses

# Build player paths from PLAYERS variable for run target
# Parser expects one -p flag per player binary
PLAYER_RUN_ARGS = $(foreach p,$(PLAYERS),-p ./$(BUILD_DIR)/player-$(p))
PLAYER_RUN_TARGETS = $(addprefix $(BUILD_DIR)/player-,$(PLAYERS))

run: $(MASTER_BIN) $(PLAYER_RUN_TARGETS) $(VIEW_BIN)
	./$(MASTER_BIN) -v ./$(VIEW_BIN) $(PLAYER_RUN_ARGS)

clean:
	rm -rf $(BUILD_DIR)

# ==== Tests (CuTest) ====
TEST_DIR         = tests
CUTEST_DIR       = $(TEST_DIR)/vendor/cutest
TEST_UNIT_DIR    = $(TEST_DIR)/unit
TEST_INCLUDE_DIR = $(TEST_DIR)/include
TEST_BUILD_DIR   = $(BUILD_DIR)/tests

# CuTest library + every unit test translation unit + test_main entrypoint.
TEST_SRCS = $(CUTEST_DIR)/CuTest.c \
            $(wildcard $(TEST_UNIT_DIR)/*.c)

# Only the project sources actually exercised by the tests. We deliberately
# avoid linking master/main.c (its own main collides with test_main) and
# anything under master/src that is currently known to have compile errors
# unrelated to the units under test. common/src/game_sync.c is pulled in so
# the game_sync unit tests can exercise the writer/reader/view/player
# helpers over an in-process game_sync_t. master/src/game_admin.c is
# pulled in for game_register_player / game_register_all / game_state_init /
# is_move_allowed / apply_move / register_move tests.
TEST_PROJECT_SRCS = master/utils/parser.c \
                    common/src/error_management.c \
                    common/src/game_sync.c \
                    common/src/player_protocol.c \
                    master/src/game_admin.c

TEST_BIN = $(TEST_BUILD_DIR)/run_tests

TEST_INCLUDES = -I$(CUTEST_DIR) -I$(TEST_INCLUDE_DIR) -I$(COMMON_INC) -Imaster/include -Imaster/utils

TEST_LDFLAGS = $(LDFLAGS)

$(TEST_BIN): $(TEST_SRCS) $(TEST_PROJECT_SRCS)
	@mkdir -p $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_INCLUDES) $^ -o $@ $(TEST_LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

# Run the unit test binary under Valgrind with strict leak checking.
# --error-exitcode=1 makes the target fail if valgrind reports any error
# or leak, so CI can gate on it without extra parsing.
memcheck: $(TEST_BIN)
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --errors-for-leak-kinds=all \
	         --track-origins=yes \
	         --error-exitcode=1 \
	         ./$(TEST_BIN)

# Generate compile_flags.txt for clangd / IDEs
compile_flags:
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-Iutils" > master/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-DGREEDY_FLOOD" > player/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" > view/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-Iinclude" > common/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-Ivendor/cutest" "-Iinclude" "-I../common/include" "-I../master/include" "-I../master/utils" > tests/compile_flags.txt
	@echo "compile_flags.txt generated for all modules"
