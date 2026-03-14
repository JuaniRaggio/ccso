# CCSO - Chomp Champs Sistemas Operativos

## Preguntas

- Para manejar los jugadores, tendriamos que recibir punteros a los jugadores desde el binario de players o
  deberiamos hacer un switch enorme y dependiendo del pid actual, obtener el player id y en base a eso hacer
  los movimientos?

- Como hacer para tener memoria compartida entre procesos? No tendria sentido enviar un puntero por un pipe
  ya que como se menciono cada proceso tiene un mapeo de memoria distinto
  
  _Se traduce todo usando shm_overview / mmap / unmap, probablemente se vea en la clase de memoria compartida_

\

# AI - Player types

- Navie (random no checks)
- BFS
- DFS
- Minimax
- A\*

## Take into account

- Si un jugador calcular que va a ir a una posicion {a, b}, podria pasar que siga calculando los siguientes
  movimientos pero eso implicaria que si justo otro jugador decide moverse a {a, b}, entonces va a cometer un
  movimiento invalido lo cual invalida todos los siguientes movimientos que queria hacer este jugador

\

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

