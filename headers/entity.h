#ifndef ENTITY_H
#define ENTITY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "header.h"

/*
Options
`bits`:  holds the OR-ed set of OPT_* flags.
`path`:  is the directory to list (defaults to ".").
*/
typedef struct {
    unsigned int bits;
    const char  *path;
} Options;

/*
Entry
One directory item. The name is copied out of readdir's internal
buffer (which is reused) and the stat result is cached so -l, -t,
and -S don't need to re-stat every entry.
*/
typedef struct {
    char         name[256];
    struct stat  st;
} Entry;

/*
EntryList
Growable array of Entry. We collect every entry first so we can
sort before printing (readdir gives no ordering guarantees).
*/
typedef struct {
    Entry *items;
    size_t count;
    size_t cap;
} EntryList;

// ---------- Public API ----------
void opt_set(Options *o, unsigned int f);        // turn a flag on
int  opt_has(const Options *o, unsigned int f);  // test a flag
int  list_dir(const char *path, const Options *opts);
void print_help(const char *prog);

#endif