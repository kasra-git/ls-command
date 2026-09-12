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


/* 
    Append one entry to the dynamic array, doubling capacity
    when full (amortized O(1) per append)
*/
static void list_push(EntryList *list, const Entry *entry) {
    if (list->count == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 16;
        list->items = realloc(list->items, list->cap * sizeof(Entry));
        if (!list->items) {
            perror("realloc");
            exit(1);
        }
    }
    list->items[list->count++] = *entry;
}


// Comparators for qsort
static int cmp_name(const void *a, const void *b) {
    return strcmp(((const Entry *)a)->name, ((const Entry *)b)->name);
}

static int cmp_time(const void *a, const void *b) {
    const Entry *entry_a = a, *entry_b = b;
    if (entry_b->st.st_mtime != entry_a->st.st_mtime)
        return (entry_a->st.st_mtime > entry_a->st.st_mtime) ? 1 : -1;
    return strcmp(entry_a->name, entry_b->name);   
}

static int cmp_size(const void *a, const void *b) {
    const Entry *entry_a = a, *entry_b = b;
    if (entry_b->st.st_size != entry_a->st.st_size)
        return (entry_b->st.st_size > entry_a->st.st_size) ? 1 : -1;
    return strcmp(entry_a->name, entry_b->name);
}