CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
  LDFLAGS += -lrt
endif

BUILD_DIR = build
COMMON_INC = common/include

STRATEGIES = 세희 胡安尼 Morena Dante el_intrepido Matias DJSanti
PLAYERS ?= $(STRATEGIES)

MASTER_SRCS = master/main.c $(wildcard master/src/*.c) $(wildcard master/utils/*.c)
PLAYER_SRCS = player/main.c $(wildcard player/src/*.c)
VIEW_SRCS   = view/main.c $(wildcard view/src/*.c)
COMMON_SRCS = $(wildcard common/src/*.c)

MASTER_BIN = $(BUILD_DIR)/master
VIEW_BIN   = $(BUILD_DIR)/view
PLAYER_BINS = $(addprefix $(BUILD_DIR)/,$(STRATEGIES))

define get_flag
$(if $(filter 세희,$(1)),GREEDY_FLOOD,\
$(if $(filter 胡安尼,$(1)),FLOOD,\
$(if $(filter Morena,$(1)),GREEDY_LOOKAHEAD,\
$(if $(filter Dante,$(1)),GREEDY,\
$(if $(filter el_intrepido,$(1)),NAIVE,\
$(if $(filter Matias,$(1)),MIN_REWARD,\
$(if $(filter DJSanti,$(1)),FLOOD)))))))
endef

DOCKER_IMAGE = agodio/itba-so-multiarch:3.1
DOCKER_RUN   = docker run --rm -v "$(CURDIR):/root/ccso" -w /root/ccso $(DOCKER_IMAGE)
DOCKER_RUN_IT = docker run --rm -it -e LC_ALL=C.UTF-8 -e LANG=C.UTF-8 \
                -v "$(CURDIR):/root/ccso" -w /root/ccso $(DOCKER_IMAGE)

DEFAULT_ARGS = -v ./$(VIEW_BIN) $(foreach p,$(STRATEGIES),-p ./$(BUILD_DIR)/$(p))

.PHONY: pull build run test memcheck pvs clean compile_flags

pull:
	docker pull $(DOCKER_IMAGE)

build:
	$(DOCKER_RUN) make _deps _all

run:
	$(DOCKER_RUN_IT) bash -c "make _deps _all && \
		./$(MASTER_BIN) $(if $(ARGS),$(ARGS),$(DEFAULT_ARGS))"

test:
	$(DOCKER_RUN) make _deps _test

memcheck:
	$(DOCKER_RUN) make _deps _memcheck

DOCKER_RUN_AMD64 = docker run --platform linux/amd64 --rm -v "$(CURDIR):/root/ccso" -w /root/ccso $(DOCKER_IMAGE)

pvs:
	$(DOCKER_RUN_AMD64) make _deps _pvs

clean:
	rm -rf $(BUILD_DIR)

compile_flags:
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-Iutils" > master/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" "-DGREEDY_FLOOD" > player/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-I../common/include" "-Iinclude" > view/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-Iinclude" > common/compile_flags.txt
	@printf "%s\n" "-Wall" "-g" "-Ivendor/cutest" "-Iinclude" "-I../common/include" "-I../master/include" "-I../master/utils" > tests/compile_flags.txt
	@echo "compile_flags.txt generated for all modules"

# ============================================================
#  Internal targets (run inside Docker container)
# ============================================================
.PHONY: _deps _all _players _test _memcheck _pvs_deps _pvs

_deps:
	@dpkg -s libncursesw5-dev > /dev/null 2>&1 || (echo "Installing libncursesw5-dev..." && apt-get update -qq && apt-get install -y -qq libncursesw5-dev)

_pvs_deps:
	@which bear > /dev/null 2>&1 || (echo "Installing bear..." && apt-get update -qq && apt-get install -y -qq bear)
	@which pvs-studio-analyzer > /dev/null 2>&1 || ( \
		echo "Installing PVS-Studio..." && \
		apt-get update -qq && \
		apt-get install -y -qq wget gnupg2 && \
		wget -q -O - https://files.pvs-studio.com/etc/pubkey.txt | gpg --dearmor -o /usr/share/keyrings/pvs-studio.gpg && \
		echo 'deb [signed-by=/usr/share/keyrings/pvs-studio.gpg] https://files.pvs-studio.com/deb viva64 main' > /etc/apt/sources.list.d/pvs-studio.list && \
		apt-get update -qq && \
		apt-get install -y -qq pvs-studio \
	)

_all: $(MASTER_BIN) _players $(VIEW_BIN)

_players: $(PLAYER_BINS)

$(PLAYER_BINS): $(BUILD_DIR)/%: $(PLAYER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -D$(call get_flag,$*) -I$(COMMON_INC) -Iplayer/include $^ -o $@ $(LDFLAGS)

$(MASTER_BIN): $(MASTER_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(COMMON_INC) -Imaster/include -Imaster/utils $^ -o $@ $(LDFLAGS)

$(VIEW_BIN): $(VIEW_SRCS) $(COMMON_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -D_GNU_SOURCE -I$(COMMON_INC) -Iview/include $^ -o $@ $(LDFLAGS) -lncursesw

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

_test: $(TEST_BIN)
	./$(TEST_BIN)

_memcheck: $(TEST_BIN)
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --errors-for-leak-kinds=all \
	         --track-origins=yes \
	         --error-exitcode=1 \
	         ./$(TEST_BIN)

PVS_NAME ?= PVS-Studio Free
PVS_KEY  ?= FREE-FREE-FREE-FREE

_pvs: _pvs_deps _deps
	pvs-studio-analyzer credentials "$(PVS_NAME)" "$(PVS_KEY)"
	./scripts/pvs-comments.sh add
	rm -rf $(BUILD_DIR)
	bear -- make _all
	pvs-studio-analyzer analyze -f compile_commands.json -o pvs-report.log -j1; \
	status=$$?; \
	./scripts/pvs-comments.sh remove; \
	plog-converter -a 'GA:1,2,3;64:1,2,3;OP:1,2,3;CS:1,2,3;MISRA:1,2,3' -t errorfile pvs-report.log; \
	exit $$status
