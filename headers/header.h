#ifndef HEADER_H
#define HEADER_H

// ---------- Standard & POSIX headers ----------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

/*
Flag bits
Each option owns exactly one bit, so any combination of flags
can be stored in a single `unsigned int`
*/
#define OPT_A    (1u << 0)   // -a   show all (including dotfiles)
#define OPT_L    (1u << 1)   // -l   long listing format
#define OPT_R    (1u << 2)   // -R   recurse into subdirectories
#define OPT_r    (1u << 3)   // -r   reverse sort order
#define OPT_T    (1u << 4)   // -t   sort by modification time (newest first)
#define OPT_S    (1u << 5)   // -S   sort by size (largest first)
#define OPT_H    (1u << 6)   // -h   human-readable sizes (used with -l)
#define OPT_i    (1u << 7)   // -i   print inode numbers
#define OPT_F    (1u << 8)   // -F   append type indicator (/, *, @)
#define OPT_1    (1u << 9)   // -1   one entry per line

// ---------- ANSI colors ----------
#define RESET      "\033[0m"
#define COLOR_DIR  "\033[34m"   // blue   -> directories
#define COLOR_FILE "\033[37m"   // white  -> regular files
#define COLOR_LINK "\033[36m"   // cyan   -> symbolic links
#define COLOR_EXEC "\033[32m"   // green  -> executables

#endif