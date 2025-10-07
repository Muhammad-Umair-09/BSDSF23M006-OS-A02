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


#define MAX_FILES 1024

struct FileEntry {
    char name[256];
    struct stat info;
};

// Helper to print file permissions like rwxr-xr-x
void print_permissions(mode_t mode) {
    char perms[11];
    perms[0] = S_ISDIR(mode) ? 'd' :
               S_ISLNK(mode) ? 'l' :
               S_ISCHR(mode) ? 'c' :
               S_ISBLK(mode) ? 'b' :
               S_ISSOCK(mode) ? 's' :
               S_ISFIFO(mode) ? 'p' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
    printf("%s", perms);
}

// Long listing format (-l)
void print_long_listing(struct FileEntry *files, int count) {
    for (int i = 0; i < count; i++) {
        struct stat *st = &files[i].info;
        struct passwd *pw = getpwuid(st->st_uid);
        struct group  *gr = getgrgid(st->st_gid);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st->st_mtime));

        print_permissions(st->st_mode);
        printf(" %3ld %-8s %-8s %8ld %s %s\n",
               (long)st->st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               (long)st->st_size,
               timebuf,
               files[i].name);
    }
}

// Default “down then across” (column by column)
void print_down_then_across(struct FileEntry *files, int count) {
    int cols = 3;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = c * rows + r;
            if (i < count)
                printf("%-25s", files[i].name);
        }
        printf("\n");
    }
}

// Horizontal “across” columns (-x)
void print_across_columns(struct FileEntry *files, int count) {
    int cols = 3;
    for (int i = 0; i < count; i++) {
        printf("%-25s", files[i].name);
        if ((i + 1) % cols == 0)
            printf("\n");
    }
    if (count % cols != 0)
        printf("\n");
}

// Core list directory function
void list_directory(const char *path, int mode_l, int mode_x) {
    DIR *dir;
    struct dirent *entry;
    struct FileEntry files[MAX_FILES];
    int count = 0;

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden files
        strcpy(files[count].name, entry->d_name);
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        if (lstat(fullpath, &files[count].info) == -1) {
            perror("lstat");
            continue;
        }
        count++;
    }

    closedir(dir);

    // Sorting alphabetically
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(files[i].name, files[j].name) > 0) {
                struct FileEntry temp = files[i];
                files[i] = files[j];
                files[j] = temp;
            }
        }
    }

    if (mode_l)
        print_long_listing(files, count);
    else if (mode_x)
        print_across_columns(files, count);
    else
        print_down_then_across(files, count);
}

int main(int argc, char *argv[]) {
    int mode_l = 0, mode_x = 0;
    const char *path = ".";

    if (argc > 1) {
        if (strcmp(argv[1], "-l") == 0)
            mode_l = 1;
        else if (strcmp(argv[1], "-x") == 0)
            mode_x = 1;
        else
            path = argv[1];
    }

    list_directory(path, mode_l, mode_x);
    return 0;
}
