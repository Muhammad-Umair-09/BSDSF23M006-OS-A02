#define _GNU_SOURCE     // Enables GNU extensions like S_ISSOCK

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // getopt(), optind
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>   // mode_t
#include <sys/stat.h>    // stat(), lstat(), S_IS*
#include <pwd.h>         // getpwuid()
#include <grp.h>         // getgrgid()
#include <time.h>        // ctime()



// Function declarations
void list_simple(const char *path);
void list_long(const char *path);
void print_permissions(mode_t mode);

int main(int argc, char *argv[]) {
    int opt;
    int long_format = 0;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    if (long_format)
        list_long(path);
    else
        list_simple(path);

    return 0;
}

// ------------------------------
// Simple Listing
// ------------------------------
void list_simple(const char *path) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.')
            printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(dir);
}

// ------------------------------
// Long Listing (-l)
// ------------------------------
void list_long(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    struct passwd *pw;
    struct group *gr;

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files
        if (entry->d_name[0] == '.')
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (lstat(fullpath, &fileStat) == -1) {
            perror("lstat");
            continue;
        }

        // File type + permissions
        print_permissions(fileStat.st_mode);

        // Number of hard links
        printf(" %ld", fileStat.st_nlink);

        // User and group
        pw = getpwuid(fileStat.st_uid);
        gr = getgrgid(fileStat.st_gid);
        printf(" %s %s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        // File size
        printf(" %5ld", fileStat.st_size);

        // Last modification time (trim newline)
        char *time_str = ctime(&fileStat.st_mtime);
        time_str[strlen(time_str) - 1] = '\0'; // remove newline
        printf(" %s", time_str);

        // File name
        printf(" %s\n", entry->d_name);
    }

    closedir(dir);
}

// ------------------------------
// Print Permissions Helper
// ------------------------------
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
