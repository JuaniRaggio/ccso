#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <parser.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "game.h"
#include "game_init.h"
#include "game_state.h"
#include "master.h"
#include "pipes.h"
#include <error_management.h>


void printGameState(int8_t board[], uint16_t height, uint16_t width, int8_t players_count, bool state);
void printBoard(int8_t board[], uint16_t height, uint16_t width); // Just for us

static volatile sig_atomic_t should_exit = 0;

static void signal_handler(int32_t sig) {
    (void)sig;
    should_exit = 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    errno = 0;
    parameters_t parameters = (parameters_t){
        .width = default_width,
        .height = default_heigh,
        .delay = default_delay,
        .timeout = default_timeout,
        .seed = time(NULL),
        .view_path = default_view_path,
    };

    parameter_status_t status = parse(argc, argv, &parameters);

    if (status != success) {
        // Manage errors using status
    }

    // TODO: usar tads para los sharedGameState y sync y unificar la inicializacion de parameters con la creacion de
    // memoria

    errno = 0;

    // Checkea errores y crea las memorias compartidas
    game_t game = new_game(master);

    // Inicializa el game_state y el game_sync
    game_init(&game, parameters.width, parameters.height, parameters.seed, parameters.players_count , parameters.players_paths);


//  -----------------------------------------------------------------------------------------------

    // Creamos los pipes, los procesos hijos, cargamos cada player y lo corremos
    int pipes[MAX_PLAYERS][2], players_count;
    players_count = game.state->players_count;

    createPipes(pipes, players_count);
    forkPlayers(pipes, players_count, game.state);
    closePipes(pipes, WRITE , players_count); // Cierro todos los writes-ends del master

// ------------------------------------------------------------------------------------------------

    /*
    IDEA: crear un masterSet y ahi cargar todos los filesDescriptors. Para el SELECT, usar el readFds, ya que el select lo
    cambia (deja unicamente los que tienen algo para leer), entonces en la proxima iteracion, lo unico que deberiamos hacer
    es readFds = masterSet
    */
    int maxFd;
    fd_set masterSet, readFds; //Creo dos cjto de fileDescriptors
    initFdSet(&masterSet, pipes, players_count, &maxFd);

}
