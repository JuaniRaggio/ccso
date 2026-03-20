#include <parser.h>
#include <stddef.h>
#include <stdint.h>

static inline size_t get_offset(char flag) {
   switch (flag) {
   case 'w':
      return offsetof(parameters_t, width);
   case 'h':
      return offsetof(parameters_t, height);
   case 'd':
      return offsetof(parameters_t, delay);
   case 't':
      return offsetof(parameters_t, timeout);
   case 's':
      return offsetof(parameters_t, seed);
   case 'v':
      return offsetof(parameters_t, view_path);
   case 'p':
      return offsetof(parameters_t, players_paths);
   }
}

static inline parameter_status_t parse_int(char argument[], uint64_t * out) {
    uint64_t accum = 0;
    for (int i = 0; argument[i]; ++i) {
        if (argument[i] < '0' || argument[i] > '9') {
            return invalid_integer_type;
        }
        accum += argument[i] - '0';
    }
    *out = accum;
}

static inline parameter_status_t parse_argument(char flag, size_t offset, char * argument, parameters_t *parameters) {
    if (flag == 'v' || flag == 'p') {
        *(char *) ((uint8_t *) parameters + offset) = argument;
    }
    return parse_int(argument, (uint64_t *)((uint8_t *) parameters + offset));
}

parameter_status_t parse(int argc, char *argv[], parameters_t *parameters) {
   parameter_status_t status = success;
   size_t last = 0;
   for (int i = 1 /* skip program name */; i < argc && status == success; ++i) {
      if (argv[i][0] == '-') {
         last = get_offset(argv[i][1]);
      } else if (last != 0) {
         /* get_offset should never return 0 */
         status |= parse_argument(argv[i - 1][1], last, argv[i], parameters);
      }
   }
}
