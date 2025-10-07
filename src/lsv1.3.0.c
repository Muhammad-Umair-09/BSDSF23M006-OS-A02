#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void print_permissions(mode_t mode);
void print_long_format(char **files, int count, const char *path);
void print_vertical(char **files, int count);
void print_horizontal(char **files, int count);
int compare_names(const void *a, const void *b);

int main(int argc, char *argv[]) {
    DIR *dir;
    struct dirent *entry;
    char **files = NULL;
    int file_count = 0;
    int capacity = 50;
    int long_format = 0, horizontal = 0;
    char *path = ".";

    files = malloc(capacity * sizeof(char *));
    if (!files) {
        perror("malloc");
        return 1;
    }

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0)
            long_format = 1;
        else if (strcmp(argv[i], "-x") == 0)
            horizontal = 1;
        else
            path = argv[i];
    }

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        free(files);
        return 1;
    }

    // --- Read directory entries into memory ---
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (file_count >= capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(char *));
            if (!files) {
                perror("realloc");
                closedir(dir);
                return 1;
            }
        }

        files[file_count] = strdup(entry->d_name);
        if (!files[file_count]) {
            perror("strdup");
            closedir(dir);
            return 1;
        }
        file_count++;
    }
    closedir(dir);

    // --- Sort alphabetically ---
    qsort(files, file_count, sizeof(char *), compare_names);

    // --- Display output ---
    if (long_format)
        print_long_format(files, file_count, path);
    else if (horizontal)
        print_horizontal(files, file_count);
    else
        print_vertical(files, file_count);

    // --- Free memory ---
    for (int i = 0; i < file_count; i++)
        free(files[i]);
    free(files);

    return 0;
}

// --- Compare function for qsort ---
int compare_names(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

// --- Permission string ---
void print_permissions(mode_t mode) {
    char type;
    if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';
    else type = '-';

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

// --- Long listing ---
void print_long_format(char **files, int count, const char *path) {
    char fullpath[512];
    for (int i = 0; i < count; i++) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, files[i]);
        struct stat st;
        if (lstat(fullpath, &st) == -1) {
            perror("lstat");
            continue;
        }

        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);
        char timebuf[64];
        strncpy(timebuf, ctime(&st.st_mtime), sizeof(timebuf));
        timebuf[strlen(timebuf) - 1] = '\0';

        print_permissions(st.st_mode);
        printf(" %2ld %s %s %8ld %s %s\n",
               (long)st.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               (long)st.st_size,
               timebuf,
               files[i]);
    }
}

// --- Vertical (default) ---
void print_vertical(char **files, int count) {
    for (int i = 0; i < count; i++)
        printf("%s\n", files[i]);
}

// --- Horizontal (-x) ---
void print_horizontal(char **files, int count) {
    int cols = 4;
    for (int i = 0; i < count; i++) {
        printf("%-20s", files[i]);
        if ((i + 1) % cols == 0 || i == count - 1)
            printf("\n");
    }
}
