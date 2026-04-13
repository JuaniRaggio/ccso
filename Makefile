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
STRATEGIES = 세희 胡安尼 Morena Dante el_intrepido Matias
PLAYERS ?= $(STRATEGIES)

# Source discovery (main.c at module root + sources in src/ and utils/)
MASTER_SRCS = master/main.c $(wildcard master/src/*.c) $(wildcard master/utils/*.c)
PLAYER_SRCS = player/main.c $(wildcard player/src/*.c)
VIEW_SRCS   = view/main.c $(wildcard view/src/*.c)
COMMON_SRCS = $(wildcard common/src/*.c)

# Binaries
MASTER_BIN = $(BUILD_DIR)/master
VIEW_BIN   = $(BUILD_DIR)/view
PLAYER_BINS = $(addprefix $(BUILD_DIR)/,$(STRATEGIES))

# Map strategy name to -D flag
define get_flag
$(if $(filter 세희,$(1)),GREEDY_FLOOD,\
$(if $(filter 胡安尼,$(1)),FLOOD,\
$(if $(filter Morena,$(1)),GREEDY_LOOKAHEAD,\
$(if $(filter Dante,$(1)),GREEDY,\
$(if $(filter el_intrepido,$(1)),NAIVE,\
$(if $(filter Matias,$(1)),MIN_REWARD))))))
endef

DOCKER_IMAGE = agodio/itba-so-multiarch:3.1
DOCKER_RUN   = docker run --rm -v "$(CURDIR):/root/ccso" -w /root/ccso $(DOCKER_IMAGE)

.PHONY: all players run clean compile_flags test memcheck deps $(STRATEGIES) \
        docker-build docker-test docker-run docker-memcheck

deps:
	@dpkg -s libncursesw5-dev > /dev/null 2>&1 || (echo "Installing libncursesw5-dev..." && apt-get update -qq && apt-get install -y -qq libncursesw5-dev)

all: deps $(MASTER_BIN) players $(VIEW_BIN)

players: $(PLAYER_BINS)

# Individual player phony targets
$(STRATEGIES): %: $(BUILD_DIR)/%

# Pattern rule: build/<strategy_name>
$(PLAYER_BINS): $(BUILD_DIR)/%: $(PLAYER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -D$(call get_flag,$*) -I$(COMMON_INC) -Iplayer/include $^ -o $@ $(LDFLAGS)

$(MASTER_BIN): $(MASTER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Imaster/include -Imaster/utils $^ -o $@ $(LDFLAGS)

$(VIEW_BIN): $(VIEW_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Iview/include $^ -o $@ $(LDFLAGS) -lncursesw

# Build player paths from PLAYERS variable for run target
# Parser expects one -p flag per player binary
PLAYER_RUN_ARGS = $(foreach p,$(PLAYERS),-p ./$(BUILD_DIR)/$(p))
PLAYER_RUN_TARGETS = $(addprefix $(BUILD_DIR)/,$(PLAYERS))

WIDTH  ?= 20
HEIGHT ?= 20

run: $(MASTER_BIN) $(PLAYER_RUN_TARGETS) $(VIEW_BIN)
	./$(MASTER_BIN) -w $(WIDTH) -h $(HEIGHT) -v ./$(VIEW_BIN) $(PLAYER_RUN_ARGS)

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
# unrelated to the units under test.
# player_movement.c is compiled with -DGREEDY so the greedy strategy is
# selected for the test binary.
TEST_PROJECT_SRCS = master/utils/parser.c \
                    common/src/argv_parser.c \
                    common/src/error_management.c \
                    common/src/game_sync.c \
                    common/src/player_protocol.c \
                    master/src/game_admin.c \
                    master/src/pipes.c \
                    player/src/player_movement.c

TEST_BIN = $(TEST_BUILD_DIR)/run_tests

TEST_INCLUDES = -I$(CUTEST_DIR) -I$(TEST_INCLUDE_DIR) -I$(COMMON_INC) -Imaster/include -Imaster/utils -Iplayer/include -DGREEDY

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

# ==== Docker wrappers (run from macOS host) ====
docker-build:
	$(DOCKER_RUN) bash -c "make clean && make deps && make all"

docker-test:
	$(DOCKER_RUN) bash -c "make clean && make deps && make build/master players build/view && make test"

docker-run:
	docker run --rm -it -e LC_ALL=C.UTF-8 -e LANG=C.UTF-8 -v "$(CURDIR):/root/ccso" -w /root/ccso $(DOCKER_IMAGE) \
		bash -c "make deps && make all && make run WIDTH=$(WIDTH) HEIGHT=$(HEIGHT) PLAYERS='$(PLAYERS)'"

docker-memcheck:
	$(DOCKER_RUN) bash -c "make clean && make deps && make build/master players && make memcheck"
