# CCSO - Chomp Champs Sistemas Operativos

## Project structure

```
ccso/
  common/                        # Shared code across all processes
    include/
      argv_parser.h
      error_management.h
      game.h
      game_state.h
      game_sync.h
      player_protocol.h
      signals.h
      shmemory_utils.h
    src/
      argv_parser.c
      error_management.c
      game.c
      game_state.c
      game_sync.c
      player_protocol.c
      signals.c
      shmemory_utils.c

  master/                        # Master process (game orchestrator)
    main.c
    include/
      game_admin.h
      master_loop.h
      pipes.h
    src/
      game_admin.c
      master_loop.c
      pipes.c
    utils/
      parser.c
      parser.h

  player/                        # Player process (AI strategies)
    main.c
    include/
      player_loop.h
      player_movement.h
    src/
      player_loop.c
      player_movement.c

  view/                          # View process (ncurses UI)
    main.c
    include/
      view.h
      view_internal.h
    src/
      view.c
      view_board.c
      view_endscreen.c
      view_panels.c

  tests/                         # CuTest unit tests
    include/
      test_suites.h
    unit/
      test_argv_parser.c
      test_error_management.c
      test_game_admin.c
      test_game_sync.c
      test_main.c
      test_parser.c
      test_player_movement.c
      test_player_protocol.c
    vendor/
      cutest/

  scripts/
    pvs-comments.sh              # PVS-Studio license comment helper

  .github/workflows/
    tests.yml
    memcheck.yml
    pvs-studio.yml
    format.yml
```

## AI - Player strategies

- Naive (random, no checks)
- Greedy (highest adjacent reward)
- Greedy Lookahead (tree search, depth 3)
- Flood (flood fill, maximizes reachable area)
- Greedy Flood (greedy + survival threshold via flood fill)

## Build & Run

All commands run from the host via Docker. No need to enter the container manually.

### Prerequisites

```bash
make pull        # download the Docker image
```

### Build

```bash
make build       # compile master, players and view
```

### Run

```bash
make run                                                        # all strategies, defaults
make run ARGS="-p ./build/Dante -p ./build/Morena"              # custom players
make run ARGS="-p ./build/Dante -w 30 -h 30"                   # custom size
```

### Tests

```bash
make test        # run CuTest suite
make memcheck    # run tests under Valgrind
```

### Static analysis

```bash
make pvs         # run PVS-Studio analysis
```

### Clean

```bash
make clean       # remove build artifacts
```

## Master flags

- `-w <width>` board width
- `-h <height>` board height
- `-d <ms>` delay between prints
- `-t <sec>` timeout without valid moves
- `-s <seed>` random seed
- `-v <path>` path to view binary
- `-p <path>` path to a player binary
