#define _GNU_SOURCE

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
#include <sys/ioctl.h>   // ioctl(), TIOCGWINSZ for terminal width
#include <termios.h>

// Function declarations
void list_simple(const char *path);
void list_long(const char *path);
void print_permissions(mode_t mode);

// ------------------------------
// MAIN FUNCTION
// ------------------------------
int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;

    // Parse -l option
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
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
// Feature 3: Column Display (Down Then Across)
// ------------------------------
void list_simple(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct winsize w;
    int term_width = 80;

    // Get terminal width
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        term_width = w.ws_col;

    dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }

    // Step 1: Gather all filenames
    char **filenames = NULL;
    int count = 0, capacity = 10, maxlen = 0;

    filenames = malloc(capacity * sizeof(char *));
    if (!filenames)
    {
        perror("malloc");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity)
        {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
            if (!filenames)
            {
                perror("realloc");
                closedir(dir);
                return;
            }
        }

        filenames[count] = strdup(entry->d_name);
        if (!filenames[count])
        {
            perror("strdup");
            closedir(dir);
            return;
        }

        int len = strlen(entry->d_name);
        if (len > maxlen)
            maxlen = len;

        count++;
    }
    closedir(dir);

    if (count == 0)
    {
        free(filenames);
        return;
    }

    // Step 2: Determine columns and rows
    int spacing = 2;
    int col_width = maxlen + spacing;
    int num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;
    int num_rows = (count + num_cols - 1) / num_cols;

    // Step 3: Print down-then-across
    for (int row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            int index = row + col * num_rows;
            if (index < count)
                printf("%-*s", col_width, filenames[index]);
        }
        printf("\n");
    }

    // Cleanup
    for (int i = 0; i < count; i++)
        free(filenames[i]);
    free(filenames);
}

// ------------------------------
// Long Listing (-l)
// ------------------------------
void list_long(const char *path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    struct passwd *pw;
    struct group *gr;

    dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (lstat(fullpath, &fileStat) == -1)
        {
            perror("lstat");
            continue;
        }

        // Permissions
        print_permissions(fileStat.st_mode);

        // Links, owner, group, size, time, name
        printf(" %ld", fileStat.st_nlink);

        pw = getpwuid(fileStat.st_uid);
        gr = getgrgid(fileStat.st_gid);
        printf(" %s %s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf(" %5ld", fileStat.st_size);

        char *time_str = ctime(&fileStat.st_mtime);
        time_str[strlen(time_str) - 1] = '\0';
        printf(" %s", time_str);

        printf(" %s\n", entry->d_name);
    }

    closedir(dir);
}

// ------------------------------
// Print Permissions Helper
// ------------------------------
void print_permissions(mode_t mode)
{
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
