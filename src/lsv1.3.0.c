#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // getopt(), optind
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>   // ioctl(), TIOCGWINSZ
#include <limits.h>      // PATH_MAX

/* --- types & prototypes --- */
typedef enum { MODE_DEFAULT, MODE_LONG, MODE_HORIZONTAL } DisplayMode;

void list_long(const char *path);
void list_columns_vertical(const char *path);
void list_columns_horizontal(const char *path);
void print_permissions(mode_t mode);

/* helpers to gather file names */
int gather_filenames(const char *dirpath, char ***out_names, int *out_count, int *out_maxlen);
void free_filenames(char **names, int count);

/* ---------------- main ---------------- */
int main(int argc, char *argv[]) {
    int opt;
    int flag_l = 0, flag_x = 0;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l': flag_l = 1; break;
            case 'x': flag_x = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [dir...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    DisplayMode mode = MODE_DEFAULT;
    if (flag_l) mode = MODE_LONG;         // -l takes precedence
    else if (flag_x) mode = MODE_HORIZONTAL;

    /* If no directories specified, use current dir */
    int dir_start = optind;
    if (dir_start >= argc) {
        /* single current directory */
        if (mode == MODE_LONG) list_long(".");
        else if (mode == MODE_HORIZONTAL) list_columns_horizontal(".");
        else list_columns_vertical(".");
    } else {
        /* multiple directories: print header for each (like ls) */
        for (int i = dir_start; i < argc; ++i) {
            const char *path = argv[i];
            if (argc - dir_start > 1) {
                printf("%s:\n", path);
            }
            if (mode == MODE_LONG) list_long(path);
            else if (mode == MODE_HORIZONTAL) list_columns_horizontal(path);
            else list_columns_vertical(path);

            if (i < argc - 1) putchar('\n'); /* blank line between directories */
        }
    }

    return 0;
}

/* ---------------- gather and free helpers ---------------- */
int gather_filenames(const char *dirpath, char ***out_names, int *out_count, int *out_maxlen) {
    DIR *dir = opendir(dirpath);
    if (!dir) return -1;

    struct dirent *entry;
    int capacity = 64;
    int count = 0;
    int maxlen = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) { closedir(dir); return -1; }

    while ((entry = readdir(dir)) != NULL) {
        /* skip hidden files for this feature */
        if (entry->d_name[0] == '.') continue;

        if (count >= capacity) {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp) {
                /* cleanup and return error */
                for (int i = 0; i < count; ++i) free(names[i]);
                free(names);
                closedir(dir);
                return -1;
            }
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        if (!names[count]) {
            for (int i = 0; i < count; ++i) free(names[i]);
            free(names);
            closedir(dir);
            return -1;
        }

        int len = (int)strlen(entry->d_name);
        if (len > maxlen) maxlen = len;
        count++;
    }

    closedir(dir);
    *out_names = names;
    *out_count = count;
    *out_maxlen = maxlen;
    return 0;
}

void free_filenames(char **names, int count) {
    if (!names) return;
    for (int i = 0; i < count; ++i) free(names[i]);
    free(names);
}

/* ---------------- vertical (down then across) ---------------- */
void list_columns_vertical(const char *path) {
    char **names = NULL;
    int count = 0, maxlen = 0;
    if (gather_filenames(path, &names, &count, &maxlen) != 0) {
        fprintf(stderr, "ls: cannot read directory '%s': %s\n", path, strerror(errno));
        return;
    }
    if (count == 0) { free_filenames(names, count); return; }

    /* get terminal width */
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) term_width = ws.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    if (col_width <= 0) col_width = 1;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            int idx = c * num_rows + r;
            if (idx < count) {
                printf("%-*s", col_width, names[idx]);
            }
        }
        putchar('\n');
    }

    free_filenames(names, count);
}

/* ---------------- horizontal (-x) ---------------- */
void list_columns_horizontal(const char *path) {
    char **names = NULL;
    int count = 0, maxlen = 0;
    if (gather_filenames(path, &names, &count, &maxlen) != 0) {
        fprintf(stderr, "ls: cannot read directory '%s': %s\n", path, strerror(errno));
        return;
    }
    if (count == 0) { free_filenames(names, count); return; }

    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) term_width = ws.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    if (col_width <= 0) col_width = 1;

    int current_width = 0;
    for (int i = 0; i < count; ++i) {
        int len = (int)strlen(names[i]);
        if (current_width + len + spacing > term_width) {
            putchar('\n');
            current_width = 0;
        }
        printf("%-*s", col_width, names[i]);
        current_width += col_width;
    }
    putchar('\n');

    free_filenames(names, count);
}

/* ---------------- long listing (-l) ---------------- */
void list_long(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) { fprintf(stderr, "ls: cannot open '%s': %s\n", path, strerror(errno)); return; }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        /* construct full path */
        char full[PATH_MAX + 1];
        if (snprintf(full, sizeof(full), "%s/%s", path, entry->d_name) >= (int)sizeof(full)) {
            /* name too long, skip */
            continue;
        }

        struct stat st;
        if (lstat(full, &st) == -1) {
            fprintf(stderr, "ls: cannot stat '%s': %s\n", full, strerror(errno));
            continue;
        }

        print_permissions(st.st_mode);
        printf(" %ld", (long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);
        printf(" %s %s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf(" %5ld", (long)st.st_size);

        char tmbuf[64];
        struct tm *tm = localtime(&st.st_mtime);
        if (tm) strftime(tmbuf, sizeof(tmbuf), "%b %e %H:%M %Y", tm);
        else tmbuf[0] = '\0';
        printf(" %s", tmbuf);

        printf(" %s\n", entry->d_name);
    }

    closedir(dir);
}

/* ---------------- permissions printer ---------------- */
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
    putchar((mode & S_IXUSR) ? ((mode & S_ISUID) ? 's' : 'x') : ((mode & S_ISUID) ? 'S' : '-'));
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? ((mode & S_ISGID) ? 's' : 'x') : ((mode & S_ISGID) ? 'S' : '-'));
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? ((mode & S_ISVTX) ? 't' : 'x') : ((mode & S_ISVTX) ? 'T' : '-'));
}
