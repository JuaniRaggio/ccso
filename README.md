# CCSO - Chomp Champs Sistemas Operativos

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
make run                                    # all strategies, 20x20 board
make run PLAYERS="Dante el_intrepido"       # subset of players
make run WIDTH=30 HEIGHT=30                 # custom board size
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

## Type lengths

> [!NOTE]
> We checked inside the docker container type lengths so that
> we can use fixed-size types to avoid ambiguity

```c
//> Inside docker
int main(void) {
	printf("Size uint: %llu\n", sizeof(unsigned int));
	printf("Size int: %llu\n", sizeof(int));
	printf("Size u short: %llu\n", sizeof(unsigned short));
	printf("Size u char: %llu\n", sizeof(unsigned char));
	printf("Size char: %llu\n", sizeof(char));
}

//> Output:
Size uint: 4
Size int: 4
Size u short: 2
Size u char: 1
```
