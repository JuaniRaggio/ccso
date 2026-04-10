# CCSO - Chomp Champs Sistemas Operativos

## Preguntas

- Para manejar los jugadores, tendriamos que recibir punteros a los jugadores desde el binario de players o
  deberiamos hacer un switch enorme y dependiendo del pid actual, obtener el player id y en base a eso hacer
  los movimientos?

- Como hacer para tener memoria compartida entre procesos? No tendria sentido enviar un puntero por un pipe
  ya que como se menciono cada proceso tiene un mapeo de memoria distinto
  
  _Se traduce todo usando shm_overview / mmap / unmap, probablemente se vea en la clase de memoria compartida_

# AI - Player strategies

- Naive (random, no checks)
- Greedy (highest adjacent reward)
- Greedy Lookahead (tree search, depth 3)
- Flood (flood fill, maximizes reachable area)
- Greedy Flood (greedy + survival threshold via flood fill)

# Build & Run

## Docker setup

### Pull the container image

```bash
docker pull agodio/itba-so-multiarch:3.1
```

### Run the container with the project mounted

From the repository root:

```bash
docker run -it --rm -v "${PWD}:/root/ccso" -w /root/ccso agodio/itba-so-multiarch:3.1 bash
```

### Build and run inside the container

Once inside the shell:

```bash
make clean
make all
make run
```

## Build individual targets

```bash
make master               # just the master
make view                 # just the view
make players              # all player strategies
make player-greedy        # a single player strategy
make player-greedy_flood  # another single strategy
```

## Run with defaults (all strategies)

```bash
make run
```

Internally this executes:

```bash
./build/master -v ./build/view \
    -p ./build/player-naive \
    -p ./build/player-greedy \
    -p ./build/player-greedy_lookahead \
    -p ./build/player-flood \
    -p ./build/player-greedy_flood
```

> [!IMPORTANT]
> The parser expects one `-p` flag per player binary. Do not put multiple
> paths after a single `-p`.

## Run with a subset of players

The `PLAYERS` variable controls which strategies are launched:

```bash
make run PLAYERS="naive greedy"        # only 2 players
make run PLAYERS="greedy_flood"        # single-player run
make run PLAYERS="flood greedy_flood"  # compare two strategies
```

## Master flags

- `-w <width>` board width
- `-h <height>` board height
- `-d <ms>` delay between prints
- `-t <sec>` timeout without valid moves
- `-s <seed>` random seed
- `-v <path>` path to view binary
- `-p <path>` path to a player binary

## Take into account

- Si un jugador calcular que va a ir a una posicion {a, b}, podria pasar que siga calculando los siguientes
  movimientos pero eso implicaria que si justo otro jugador decide moverse a {a, b}, entonces va a cometer un
  movimiento invalido lo cual invalida todos los siguientes movimientos que queria hacer este jugador

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

