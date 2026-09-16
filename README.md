# ls

A minimal `ls` implementation written in C from scratch.

Built to understand how directory listing works at the system-call level:
`opendir`, `readdir`, `lstat`, and the POSIX permission model — without
relying on any shelling out to the real `ls`.

---

## Table of Contents

- [Features](#features)
- [Demo](#demo)
- [Build](#build)
- [Usage](#usage)
- [How It Works](#how-it-works)
- [Project Structure](#project-structure)
- [Design Notes](#design-notes)
- [What's Not Implemented](#whats-not-implemented)

---

## Features

| Flag | Description |
|------|-------------|
| `-a` | Show hidden files (names starting with `.`) |
| `-l` | Long listing format (permissions, owner, size, date) |
| `-R` | Recurse into subdirectories |
| `-r` | Reverse the sort order |
| `-t` | Sort by modification time, newest first |
| `-S` | Sort by file size, largest first |
| `-h` | Human-readable sizes (`1.2K`, `3.4M`) — used with `-l` |
| `-i` | Print inode numbers |
| `-F` | Append type indicator: `/` dir, `*` executable, `@` symlink |
| `--help` | Show usage |

Flags can be combined freely (`-la`, `-al`, `-l -a` all work).

Output is colorized by file type when writing to a terminal:

- **Blue** — directories
- **White** — regular files
- **Cyan** — symbolic links
- **Green** — executables

Colors are automatically disabled when stdout is not a TTY, so piping
to `less` or redirecting to a file produces clean, escape-free text.

---

## Demo

```console
$ ./ls -l
drwxr-xr-x  5 bob  users  4.0K Sep 10 14:22 Documents
drwxr-xr-x  3 bob  users  4.0K Sep 10 12:01 Downloads
-rw-r--r--  1 bob  users   220 Sep 09 09:14 notes.txt
-rwxr-xr-x  1 bob  users  1.4M Sep 08 18:33 myls

$ ./ls -lah /tmp
drwxrwxrwt 12 root root  4.0K Sep 10 14:22 .
drwxr-xr-x 20 root root  4.0K Sep 10 12:00 ..
-rw-r--r--  1 bob  users  12   Sep 10 13:45 session.log
```

---

## Build

Requires a C99 compiler and POSIX. On Linux or macOS:

```bash
make
```

This produces a `ls` binary in the project root.

To build manually without `make`:

```bash
gcc -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 \ main.c src/src.c -o ls
```

---

## Usage

```
./ls [OPTION]... [FILE]...
```

With no arguments, lists the current directory. With a path, lists that
directory.

```bash
./ls                # list current directory
./ls -la            # long format, including hidden files
./ls -lh /var/log   # human-readable sizes
./ls -lR /tmp       # recurse through /tmp
./ls --help         # show usage
```

---

## How It Works

The program runs in two phases: **parse arguments**, then **walk the
directory**.

### Phase 1 — Argument parsing (`main.c`)

Every flag is stored as a single bit inside one `unsigned int`:

```c
#define OPT_A  (1u << 0)   // -a
#define OPT_L  (1u << 1)   // -l
#define OPT_R  (1u << 2)   // -R
// ...
```

`getopt_long()` walks `argv` and, for each option it finds, calls
`opt_set()` to turn on the matching bit. Because each flag owns a
distinct bit, `-la`, `-al`, and `-l -a` all end up setting the same two
bits — combination works automatically.

After the parse loop, `optind` points at the first non-option argument.
If there is one, it becomes the path to list; otherwise the path defaults
to `"."`. Control is then handed to `list_dir()`.

### Phase 2 — Directory listing (`src.c`)

`list_dir()` performs six steps:

1. **Open.** `opendir()` opens the target path. On failure, `perror()`
   prints the error and the function returns `-1`.

2. **Collect.** A loop calls `readdir()` until it returns `NULL`. For
   every name:
   - Skip dotfiles unless `-a` was given.
   - Build the full path (`path/name`) so `lstat()` resolves relative
     to the target directory, not the process's working directory.
   - Call `lstat()` to fetch permissions, size, owner, group, and
     mtime, and push the result onto a growable `EntryList`.

   > **Why collect first?** `readdir()` gives no ordering guarantee,
   > and `-t`, `-S`, and `-r` all need the complete set of entries
   > before anything can be printed.

3. **Sort.** A comparator is chosen based on the flags:
   `cmp_time` for `-t`, `cmp_size` for `-S`, otherwise `cmp_name`.
   `qsort()` sorts in place. If `-r` is set, the sorted array is
   reversed.

4. **Print.** For each entry, `print_entry()` emits:
   - The inode number, if `-i`.
   - For `-l`, the full row: permissions, link count, owner, group,
     size (human-readable if `-h`), date, and name.
   - For default mode, just the name.
   - The name is colored by file type — but only if stdout is a TTY.
   - A type suffix (`/`, `*`, `@`) if `-F`.

5. **Recurse.** If `-R` is set, walk the list again and call
   `list_dir()` on every subdirectory (skipping `.` and `..`), printing
   a header above each.

6. **Clean up.** Free the entry list and return `0`.

---

## Project Structure

```
ls-command-line-tool
├── main.c            # entry point, argument parsing
├── src/  
│   └── src.c         # listing logic, formatting, recursion
├── headers/
│   ├── header.h      # standard includes, flag bits, color macros
│   └── entity.h      # Options, Entry, EntryList, public API
└── Makefile
```

The public API (`list_dir`, `opt_set`, `opt_has`, `print_help`) is
declared in `entity.h` and defined in `src.c`. `main.c` never touches
the internals — it just sets flags and calls `list_dir`.

---

## Design Notes

A few decisions worth calling out:

- **Flags as bits.** Storing all options in one integer makes passing
  them around trivial and adding a new flag a one-line change. The
  `Options` struct bundles the flag bits and the target path.

- **`lstat` instead of `stat`.** `stat()` follows symlinks and describes
  what they point at. `lstat()` describes the link itself, which is what
  real `ls -l` shows (`lrwxrwxrwx ... -> target`).

- **Collect, sort, then print.** Streaming output directly from
  `readdir()` would be simpler, but it makes sorting impossible. A
  dynamic array lets `-t`, `-S`, and `-r` work naturally.

- **Color only on TTYs.** `isatty(STDOUT_FILENO)` gates the ANSI escape
  codes. Without this, `./main > out.txt` would write raw `\033[34m`
  bytes into the file.

- **No global state.** Every function receives what it needs through
  parameters. `Options` is passed by `const` pointer; the entry list is
  local to `list_dir`.

---

## What's Not Implemented

Compared to GNU `ls`, this is still minimal. Missing pieces, roughly in
order of usefulness:

- **Multi-column output** (the default for terminals) — requires
  `ioctl(TIOCGWINSZ)` and a grid-layout pass.
- **Column alignment in `-l`** — currently uses fixed widths, so long
  owner names or sizes can push the row out of line. A two-pass approach
  (measure, then print) fixes it.
- **Cycle detection for `-R`** — symlink loops are avoided because
  `lstat` never descends into them, but a broader visited-inode set
  would be needed if `-L` (follow symlinks) were added.
- **Sorting files before directories** — real `ls` prints non-directory
  arguments first, then each directory with a header.
- **`--color=auto|always|never`** — currently hard-coded to `auto`.
