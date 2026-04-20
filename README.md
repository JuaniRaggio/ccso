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

All project targets run inside the Docker container (`agodio/itba-so-multiarch:3.1`).

### Enter the container

```bash
make docker          # enter the container (arm64)
make docker-pvs      # enter the container (amd64, for PVS-Studio)
```

### Inside the container

```bash
make install         # install dependencies
make build           # install + compile everything
make run             # build + run with all strategies
make test            # compile and run CuTest suite
make memcheck        # run tests under Valgrind
make best_player     # compile best strategy (세희) as build/best_player
make clean           # remove build artifacts
```

### Custom game

```bash
make run ARGS="-w 20 -h 20 -p ./build/Dante -p ./build/Morena"
make run ARGS="-w 30 -h 30 -p ./build/세희 -p ./build/Dante -p ./build/el_intrepido"
```

### Static analysis (PVS-Studio)

```bash
make clean
make pvs
```

## Known issues

There is a small delay at the end of the game before the endscreen appears.
This is likely caused by the master waiting for the inactivity timeout to
expire after all players have finished, but we are not 100% sure of the
exact cause.

## Master flags

- `-w <width>` board width
- `-h <height>` board height
- `-d <ms>` delay between prints
- `-t <sec>` timeout without valid moves
- `-s <seed>` random seed
- `-v <path>` path to view binary
- `-p <path>` path to a player binary
