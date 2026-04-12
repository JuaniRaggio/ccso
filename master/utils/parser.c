#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <parser.h>

// We expect compiler to put this in .rodata
static const uint8_t default_base = 10;
static const int8_t parsing_succeeded = -1;
static const char *const allowed_flags = "w:h:d:t:s:v:p:";

/*
 * parse_argument expects that arguments are not destroyed before the program finishes, this limitation
 * is produced by the fact we are not allocating memory to save paths
 */
static inline void parse_argument(int opt, parameters_t *parameters, parameter_status_t *status);

static inline void parse_argument(int opt, parameters_t *parameters, parameter_status_t *status) {
    char *endptr = NULL;
    uint64_t *current_parameter = NULL;
    switch (opt) {
    case 'w':
        parameters->c_width = optarg;
        current_parameter = &parameters->width;
        goto integer_checking;
    case 'h':
        parameters->c_height = optarg;
        current_parameter = &parameters->height;
        goto integer_checking;
    case 'd':
        current_parameter = &parameters->delay;
        goto integer_checking;
    case 't':
        current_parameter = &parameters->timeout;
        goto integer_checking;
    case 's':
        current_parameter = &parameters->seed;
        goto integer_checking;

    integer_checking:
        errno = 0;
        *current_parameter = strtoull(optarg, &endptr, default_base);
        *status |= endptr == NULL || *endptr != '\0' ? invalid_integer_type : success;
        *status |= errno == ERANGE ? overflow : success;
        break;

    case 'v':
        parameters->view_path = optarg;
        break;
    case 'p':
        if (parameters->players_count == MAX_PLAYERS) {
            *status |= exceeded_player_limit;
            break;
        }
        parameters->players_paths[parameters->players_count++] = optarg;
        break;
    case '?':
    default:
        *status |= unknown_optional_flag;
    }
    // TODO: si no te pasan vista, no tiene sentido que te pasen delay
}

parameter_status_t parse(int argc, char *argv[], parameters_t *parameters) {
    errno = 0;
    parameter_status_t status = success;
    int opt = 0;
    while ((opt = getopt(argc, argv, allowed_flags)) != parsing_succeeded && status == success) {
        parse_argument(opt, parameters, &status);
    }
    return status;
}
