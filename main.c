#define _POSIX_C_SOURCE 200809L
#include <getopt.h>
#include "headers/header.h"
#include "headers/entity.h"


int main(int argc, char *argv[]) {
    Options opts = { .bits = 0, .path = "." };

    // Long options. --help maps to the same handler as -?.
    static struct option long_opts[] = {
        { "help", no_argument, 0, '?' },
        { 0, 0, 0, 0 }                 // terminator
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "alRrthSiF1?", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'a': opt_set(&opts, OPT_A); break;
        case 'l': opt_set(&opts, OPT_L); break;
        case 'R': opt_set(&opts, OPT_R); break;
        case 'r': opt_set(&opts, OPT_r); break;
        case 't': opt_set(&opts, OPT_T); break;
        case 'S': opt_set(&opts, OPT_S); break;
        case 'h': opt_set(&opts, OPT_H); break;
        case 'i': opt_set(&opts, OPT_i); break;
        case 'F': opt_set(&opts, OPT_F); break;
        case '1': opt_set(&opts, OPT_1); break;

        case '?':
            if (optopt == '?' || optopt == 0) {
                print_help(argv[0]);
                return 0;
            }
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return 1;

        default:
            fprintf(stderr, "Usage: %s [-alRrthSiF1] [path]\n", argv[0]);
            return 1;
        }
    }

    // Anything after the options is the path (if provided).
    if (optind < argc)
        opts.path = argv[optind];

    return list_dir(opts.path, &opts) == 0 ? 0 : 1;
}