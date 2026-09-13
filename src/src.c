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


/*
Build the 10-character permission string.
out[0] is the file type; out[1..9] are user/group/other rwx bits.
*/
static void mode_string(mode_t m, char out[11]) {
    strcpy(out, "----------");

    if (S_ISDIR(m))  out[0] = 'd';
    if (S_ISLNK(m))  out[0] = 'l';
    if (S_ISCHR(m))  out[0] = 'c';
    if (S_ISBLK(m))  out[0] = 'b';
    if (S_ISFIFO(m)) out[0] = 'p';
    if (S_ISSOCK(m)) out[0] = 's';

    if (m & S_IRUSR) out[1] = 'r';
    if (m & S_IWUSR) out[2] = 'w';
    if (m & S_IXUSR) out[3] = 'x';
    if (m & S_IRGRP) out[4] = 'r';
    if (m & S_IWGRP) out[5] = 'w';
    if (m & S_IXGRP) out[6] = 'x';
    if (m & S_IROTH) out[7] = 'r';
    if (m & S_IWOTH) out[8] = 'w';
    if (m & S_IXOTH) out[9] = 'x';
}

/*
Format `size` as "1.2K", "3.4M", into `buffer`.
Used only when OPT_H(-h) is set.
*/
static void print_human(long size, char *buffer, size_t length) {
    const char *units[] = {"B", "K", "M", "G", "T"};
    double double_size = size;
    int counter = 0;

    while (double_size >= 1024.0 && counter < 4) {
        double_size /= 1024.0;
        counter++;
    }
    
    if (counter == 0) 
        snprintf(buffer, length, "%ld", size);
    else        
        snprintf(buffer, length, "%.1f%s", double_size, units[counter]);
}

/*
Choose a color based on the entry's type.
Order matters: a directory wins over "executable", ...
*/
static const char *color_for(const Entry *entry) {
    if (S_ISDIR(entry->st.st_mode))       
        return COLOR_DIR;
    if (S_ISLNK(entry->st.st_mode))       
        return COLOR_LINK;
    if (entry->st.st_mode & S_IXUSR)      
        return COLOR_EXEC;
    return COLOR_FILE;
}

// Only emit ANSI codes when stdout is an actual terminal.
static int use_color(void) {
    return isatty(STDOUT_FILENO);
}