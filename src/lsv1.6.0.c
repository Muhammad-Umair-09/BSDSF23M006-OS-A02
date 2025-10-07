#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/param.h>

/* ANSI color codes required by assignment */
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE  "\033[1;34m"   /* directories */
#define COLOR_GREEN "\033[1;32m"   /* executables */
#define COLOR_RED   "\033[1;31m"   /* archives (.tar, .gz, .zip) */
#define COLOR_PINK  "\033[1;35m"   /* symbolic links */
#define COLOR_REV   "\033[7m"      /* special files (device, socket, fifo) */

/* Display modes */
enum DisplayMode { DEFAULT_MODE, LONG_MODE, HORIZONTAL_MODE };

/* Function declarations */
void list_files(const char *path, enum DisplayMode mode, int recursive);
void print_permissions(mode_t mode);
void print_long_listing(char **names, int count, const char *path);
void print_vertical_columns(char **names, int count, int maxlen, int termwidth, const char *path);
void print_horizontal_columns(char **names, int count, int maxlen, int termwidth, const char *path);
void print_colored_filename(const char *fullpath, const char *filename);
int name_compare(const void *a, const void *b);
static bool has_suffix(const char *name, const char *suf);

/* MAIN */
int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;
    int recursive = 0;

    /* Only parse flags required by assignment: -l, -x, -R */
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l':
                mode = LONG_MODE;
                break;
            case 'x':
                mode = HORIZONTAL_MODE;
                break;
            case 'R':
                recursive = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x | -R] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";
    list_files(path, mode, recursive);
    return 0;
}

/* Read directory, collect names, sort, display and optionally recurse */
void list_files(const char *path, enum DisplayMode mode, int recursive) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror(path);
        return;
    }

    /* Print header like ls -R prints each directory name */
    printf("\n%s:\n", path);

    struct dirent *entry;
    char **names = NULL;
    int count = 0;
    int capacity = 64;

    names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* Skip hidden files by default (assignment behavior) */
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                /* cleanup before returning */
                for (int i = 0; i < count; ++i) free(names[i]);
                free(names);
                closedir(dir);
                return;
            }
            names = tmp;
        }
        names[count++] = strdup(entry->d_name);
        if (!names[count-1]) {
            perror("strdup");
            /* cleanup */
            for (int i = 0; i < count-1; ++i) free(names[i]);
            free(names);
            closedir(dir);
            return;
        }
    }
    closedir(dir);

    /* Sort alphabetically */
    qsort(names, count, sizeof(char *), name_compare);

    /* Get terminal width */
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        w.ws_col = 80; /* fallback */
    }
    int termwidth = (w.ws_col > 0) ? w.ws_col : 80;

    /* Find longest name for column calculations */
    int maxlen = 0;
    for (int i = 0; i < count; ++i) {
        int len = strlen(names[i]);
        if (len > maxlen) maxlen = len;
    }

    /* Display */
    if (mode == LONG_MODE) {
        print_long_listing(names, count, path);
    } else if (mode == HORIZONTAL_MODE) {
        print_horizontal_columns(names, count, maxlen, termwidth, path);
    } else {
        print_vertical_columns(names, count, maxlen, termwidth, path);
    }

    /* If recursive, iterate entries and recurse into directories (skip symlinks) */
    if (recursive) {
        for (int i = 0; i < count; ++i) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);

            struct stat st;
            if (lstat(fullpath, &st) == -1) {
                /* ignore unreadable entries */
                continue;
            }

            /* skip symlinks to avoid following link loops */
            if (S_ISLNK(st.st_mode))
                continue;

            if (S_ISDIR(st.st_mode)) {
                /* skip . and .. (though we already filtered hidden names, this is safe) */
                if (strcmp(names[i], ".") == 0 || strcmp(names[i], "..") == 0)
                    continue;
                list_files(fullpath, mode, recursive);
            }
        }
    }

    /* cleanup */
    for (int i = 0; i < count; ++i) free(names[i]);
    free(names);
}

/* Comparator for qsort */
int name_compare(const void *a, const void *b) {
    const char *A = *(const char **)a;
    const char *B = *(const char **)b;
    return strcmp(A, B);
}

/* Print permissions string like ls -l */
void print_permissions(mode_t mode) {
    char type = '?';
    if (S_ISREG(mode)) type = '-';
    else if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';

    putchar(type);
    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
}

/* Print -l formatted listing */
void print_long_listing(char **names, int count, const char *path) {
    struct stat st;
    for (int i = 0; i < count; ++i) {
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);

        if (lstat(fullpath, &st) == -1) {
            perror("lstat");
            continue;
        }

        /* permissions and type */
        print_permissions(st.st_mode);

        /* link count, owner, group, size */
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        long nlink = (long) st.st_nlink;
        long size  = (long) st.st_size;

        printf(" %2ld %s %s %6ld ", nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               size);

        /* modification time */
        char *mtime = ctime(&st.st_mtime);
        if (mtime) {
            /* remove trailing newline from ctime */
            mtime[strlen(mtime) - 1] = '\0';
            printf("%s ", mtime);
        } else {
            printf("??? ");
        }

        /* filename (with color) */
        print_colored_filename(fullpath, names[i]);

        /* if symlink, show target */
        if (S_ISLNK(st.st_mode)) {
            char target[PATH_MAX];
            ssize_t len = readlink(fullpath, target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                printf(" -> %s", target);
            }
        }

        printf("\n");
    }
}

/* Default vertical column display (down then across) */
void print_vertical_columns(char **names, int count, int maxlen, int termwidth, const char *path) {
    int spacing = 2;
    int col_width = maxlen + spacing;
    if (col_width <= 0) col_width = 1;
    int cols = termwidth / col_width;
    if (cols <= 0) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx < count) {
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[idx]);
                print_colored_filename(fullpath, names[idx]);

                int pad = col_width - (int)strlen(names[idx]);
                if (pad < 0) pad = 0;
                for (int p = 0; p < pad; ++p) putchar(' ');
            }
        }
        putchar('\n');
    }
}

/* Horizontal (across) display (-x) */
void print_horizontal_columns(char **names, int count, int maxlen, int termwidth, const char *path) {
    (void)maxlen;
    int spacing = 2;
    int current_width = 0;

    for (int i = 0; i < count; ++i) {
        int len = (int)strlen(names[i]);
        /* if printing this would exceed terminal width, newline first */
        if (current_width + len + spacing > termwidth) {
            putchar('\n');
            current_width = 0;
        }

        /* print colored name */
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);
        print_colored_filename(fullpath, names[i]);

        /* spacing */
        for (int s = 0; s < spacing; ++s) putchar(' ');
        current_width += len + spacing;
    }
    putchar('\n');
}

/* Helper: print filename with color rules per assignment */
void print_colored_filename(const char *fullpath, const char *filename) {
    struct stat st;
    /* We use lstat() here to detect symbolic links explicitly (they must be pink).
       For non-links, the same lstat result gives file type for coloring. */
    if (lstat(fullpath, &st) == -1) {
        /* fallback: print name uncolored */
        printf("%s", filename);
        return;
    }

    if (S_ISLNK(st.st_mode)) {
        printf(COLOR_PINK "%s" COLOR_RESET, filename);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        printf(COLOR_BLUE "%s" COLOR_RESET, filename);
        return;
    }

    /* executable: check execute bits and regular file */
    if (S_ISREG(st.st_mode) && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        printf(COLOR_GREEN "%s" COLOR_RESET, filename);
        return;
    }

    /* special files: device, fifo, socket */
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode)) {
        printf(COLOR_REV "%s" COLOR_RESET, filename);
        return;
    }

    /* archives by suffix (check last '.' suffix only) */
    if (has_suffix(filename, ".tar") || has_suffix(filename, ".tgz") ||
        has_suffix(filename, ".gz")  || has_suffix(filename, ".zip")) {
        printf(COLOR_RED "%s" COLOR_RESET, filename);
        return;
    }

    /* default: regular (no color) */
    printf("%s", filename);
}

/* Returns true if name ends with suf (case-sensitive), safe for short names */
static bool has_suffix(const char *name, const char *suf) {
    if (!name || !suf) return false;
    size_t nlen = strlen(name);
    size_t slen = strlen(suf);
    if (slen > nlen) return false;
    return strcmp(name + nlen - slen, suf) == 0;
}
