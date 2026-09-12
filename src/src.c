#include "../headers/entity.h"
#include "../headers/header.h"

// Turn on the bits in `f` (each bit = one flag).
void opt_set(Options *option, unsigned int flag) {
    option->bits |= flag;
}

// Non-zero if any bit in `f` is set.
int opt_has(const Options *option, unsigned int flag) {
    return option->bits & flag;
}

// Print the --help screen
void print_help(const char *prog) {
    printf("Usage: %s [OPTION]... [FILE]...\n", prog);
    printf("List information about the FILEs (the current directory by default).\n\n");
    printf("  -a              do not ignore entries starting with .\n");
    printf("  -l              use a long listing format\n");
    printf("  -R              list subdirectories recursively\n");
    printf("  -r              reverse order while sorting\n");
    printf("  -t              sort by modification time, newest first\n");
    printf("  -S              sort by file size, largest first\n");
    printf("  -h              with -l, print sizes in human readable format\n");
    printf("  -i              print the index number of each file\n");
    printf("  -F              append indicator (one of */@) to entries\n");
    printf("  -? / --help     display this help and exit\n");
}


