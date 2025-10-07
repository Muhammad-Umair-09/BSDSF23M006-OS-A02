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

// ========== ANSI Color Codes ==========
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"   // Directories
#define COLOR_GREEN   "\033[1;32m"   // Executables
#define COLOR_RED     "\033[1;31m"   // Archives
#define COLOR_PINK    "\033[1;35m"   // Symbolic Links
#define COLOR_REV     "\033[7m"      // Special files (reverse video)

// ========== Function Declarations ==========
void list_files(const char *path, int mode);
void print_permissions(mode_t mode);
void print_long_listing(char **names, int count, const char *path);
void print_vertical_columns(char **names, int count, int maxlen, int termwidth, const char *path);
void print_horizontal_columns(char **names, int count, int maxlen, int termwidth, const char *path);
void print_colored_filename(const char *path, const char *filename);
int name_compare(const void *a, const void *b);

// Display modes
enum DisplayMode { DEFAULT_MODE, LONG_MODE, HORIZONTAL_MODE };

// =======================================================
// MAIN FUNCTION
// =======================================================
int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;

    // Parse command line options
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                mode = LONG_MODE;
                break;
            case 'x':
                mode = HORIZONTAL_MODE;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";
    list_files(path, mode);

    return 0;
}

// =======================================================
// READ DIRECTORY, SORT, AND DISPLAY
// =======================================================
void list_files(const char *path, int mode) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    char **names = NULL;
    int count = 0;
    int capacity = 50;

    names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;  // Skip hidden files

        if (count >= capacity) {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char *));
            if (!names) {
                perror("realloc");
                closedir(dir);
                return;
            }
        }
        names[count++] = strdup(entry->d_name);
    }
    closedir(dir);

    // Sort alphabetically
    qsort(names, count, sizeof(char *), name_compare);

    // Get terminal width
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int termwidth = w.ws_col ? w.ws_col : 80;

    // Find longest name
    int maxlen = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(names[i]);
        if (len > maxlen) maxlen = len;
    }

    // Display according to mode
    if (mode == LONG_MODE)
        print_long_listing(names, count, path);
    else if (mode == HORIZONTAL_MODE)
        print_horizontal_columns(names, count, maxlen, termwidth, path);
    else
        print_vertical_columns(names, count, maxlen, termwidth, path);

    // Free memory
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

// =======================================================
// COMPARISON FUNCTION FOR QSORT
// =======================================================
int name_compare(const void *a, const void *b) {
    const char *nameA = *(const char **)a;
    const char *nameB = *(const char **)b;
    return strcmp(nameA, nameB);
}

// =======================================================
// PRINT PERMISSIONS (USED IN -l MODE)
// =======================================================
void print_permissions(mode_t mode) {
    char type;
    if (S_ISREG(mode)) type = '-';
    else if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';
    else type = '?';

    printf("%c", type);
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

// =======================================================
// PRINT LONG LISTING (-l)
// =======================================================
void print_long_listing(char **names, int count, const char *path) {
    struct stat st;

    for (int i = 0; i < count; i++) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);

        if (lstat(fullpath, &st) == -1) {
            perror("lstat");
            continue;
        }

        print_permissions(st.st_mode);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);

        printf(" %2ld %s %s %6ld ", 
               (long)st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               (long)st.st_size);

        char *mtime = ctime(&st.st_mtime);
        mtime[strlen(mtime) - 1] = '\0';
        printf("%s ", mtime);

        print_colored_filename(fullpath, names[i]);
        printf("\n");
    }
}

// =======================================================
// PRINT VERTICAL (DEFAULT) MODE
// =======================================================
void print_vertical_columns(char **names, int count, int maxlen, int termwidth, const char *path) {
    int cols = termwidth / (maxlen + 2);
    if (cols == 0) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx < count) {
                print_colored_filename(path, names[idx]);
                int pad = maxlen - strlen(names[idx]) + 2;
                for (int p = 0; p < pad; p++) printf(" ");
            }
        }
        printf("\n");
    }
}

// =======================================================
// PRINT HORIZONTAL (-x MODE)
// =======================================================
void print_horizontal_columns(char **names, int count, int maxlen, int termwidth, const char *path) {
    (void)maxlen;
    int current_width = 0;

    for (int i = 0; i < count; i++) {
        int len = strlen(names[i]);
        if (current_width + len + 2 > termwidth) {
            printf("\n");
            current_width = 0;
        }

        print_colored_filename(path, names[i]);
        printf("  ");
        current_width += len + 2;
    }
    printf("\n");
}

// =======================================================
// COLORIZED PRINTING BASED ON FILE TYPE
// =======================================================
void print_colored_filename(const char *path, const char *filename) {
    char fullpath[1024];
    struct stat st;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &st) == -1) {
        printf("%s", filename);
        return;
    }

    // Apply colors based on file type
    if (S_ISDIR(st.st_mode))
        printf(COLOR_BLUE "%s" COLOR_RESET, filename);
    else if (S_ISLNK(st.st_mode))
        printf(COLOR_PINK "%s" COLOR_RESET, filename);
    else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
        printf(COLOR_REV "%s" COLOR_RESET, filename);
    else if (st.st_mode & S_IXUSR)
        printf(COLOR_GREEN "%s" COLOR_RESET, filename);
    else if (strstr(filename, ".tar") || strstr(filename, ".gz") || strstr(filename, ".zip"))
        printf(COLOR_RED "%s" COLOR_RESET, filename);
    else
        printf("%s", filename);
}
