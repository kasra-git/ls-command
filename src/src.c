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


// Forward declaration: defined below, called from list_dir.
static void print_entry(const Entry *entry, const Options *options);

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


/*
Print one entry in the -l style:
perms links owner group size date name[F-suffix]
*/
static void print_long(const Entry *entry, const Options *options) {
    char perms[11];
    mode_string(entry->st.st_mode, perms);

    // Look up names for the numeric uid/gid. Copy immediately —
    // getpwuid / getgrgid return static buffers that get reused.
    struct passwd *pw = getpwuid(entry->st.st_uid);
    struct group  *gr = getgrgid(entry->st.st_gid);
    char owner[32], group[32];
    snprintf(owner, sizeof owner, "%s", pw ? pw->pw_name : "?");
    snprintf(group, sizeof group, "%s", gr ? gr->gr_name : "?");

    // Size column (human-readable if -h was given).
    char sizebuf[32];
    if (opt_has(options, OPT_H))
        print_human(entry->st.st_size, sizebuf, sizeof sizebuf);
    else
        snprintf(sizebuf, sizeof sizebuf, "%ld", (long)entry->st.st_size);

    // Modification time, e.g. "Sep 10 14:22".
    char timebuf[64];
    struct tm *tm = localtime(&entry->st.st_mtime);
    strftime(timebuf, sizeof timebuf, "%b %e %H:%M", tm);

    // Everything up to and including the time column.
    printf(
            "%s %3lu %-8s %-8s %8s %s ",
            perms,
            (unsigned long)entry->st.st_nlink,
            owner, group, sizebuf, timebuf
    );

    // Name (colored if stdout is a TTY).
    const char *color = use_color() ? color_for(entry) : "";
    const char *reset = use_color() ? RESET         : "";
    printf("%s%s%s", color, entry->name, reset);

    // -F suffix, if requested.
    if (opt_has(options, OPT_F)) {
        if (S_ISDIR(entry->st.st_mode))       
            putchar('/');
        else if (entry->st.st_mode & S_IXUSR) 
            putchar('*');
        else if (S_ISLNK(entry->st.st_mode))  
            putchar('@');
    }
    putchar('\n');
}

/*
Dispatch for a single entry: inode column (if -i),
then either long format or a plain colored name.
*/
static void print_entry(const Entry *entry, const Options *options) {
    if (opt_has(options, OPT_i))
        printf("%lu ", (unsigned long)entry->st.st_ino);

    if (opt_has(options, OPT_L)) {
        print_long(entry, options);
        return;
    }

    // Short format: name, optional -F suffix, newline.
    const char *color = use_color() ? color_for(entry) : "";
    const char *reset = use_color() ? RESET         : "";
    printf("%s%s%s", color, entry->name, reset);

    if (opt_has(options, OPT_F)) {
        if (S_ISDIR(entry->st.st_mode))       
            putchar('/');
        else if (entry->st.st_mode & S_IXUSR) 
            putchar('*');
        else if (S_ISLNK(entry->st.st_mode))  
            putchar('@');
    }
    putchar('\n');
}

/*
Flow:
    1. open the directory
    2. read every entry, stat it, push onto a list
    3. sort the list (name / mtime / size, then optional reverse)
    4. print each entry
    5. if -R, recurse into subdirectories
*/
int list_dir(const char *path, const Options *options) {
    DIR *pDir = opendir(path);
    if (!pDir) {
        perror(path);
        return -1;
    }

    // collect entries
    EntryList list = {0};
    struct dirent *de;

    while ((de = readdir(pDir)) != NULL) {
        // Skip dotfiles unless -a was given.
        if (!opt_has(options, OPT_A) && de->d_name[0] == '.')
            continue;

        Entry e;
        snprintf(e.name, sizeof e.name, "%s", de->d_name);

        // Build a full path so lstat resolves relative to `path`,
        char full[4096];
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);

        // lstat (not stat) so symlinks themselves are described,
        if (lstat(full, &e.st) == -1) {
            perror(full);
            continue;
        }
        list_push(&list, &e);
    }
    closedir(pDir);

    // sort 
    int (*cmp)(const void *, const void *) = cmp_name;
    if (opt_has(options, OPT_T))      
        cmp = cmp_time;
    else if (opt_has(options, OPT_S)) 
        cmp = cmp_size;

    qsort(list.items, list.count, sizeof(Entry), cmp);

    // -r reverses whatever order we just established.
    if (opt_has(options, OPT_r)) {
        for (size_t i = 0, j = list.count ? list.count - 1 : 0; i < j; i++, j--) {
            Entry tmp       = list.items[i];
            list.items[i]   = list.items[j];
            list.items[j]   = tmp;
        }
    }

    // print
    for (size_t i = 0; i < list.count; i++)
        print_entry(&list.items[i], options);

    // recurse (-R)
    if (opt_has(options, OPT_R)) {
        for (size_t i = 0; i < list.count; i++) {
            if (!S_ISDIR(list.items[i].st.st_mode)) 
                continue;
            if (strcmp(list.items[i].name, ".") == 0)  
                continue;
            if (strcmp(list.items[i].name, "..") == 0) 
                continue;

            char sub[4096];
            snprintf(sub, sizeof sub, "%s/%s", path, list.items[i].name);

            printf("\n%s:\n", sub);
            list_dir(sub, options);
        }
    }

    free(list.items);
    return 0;
}