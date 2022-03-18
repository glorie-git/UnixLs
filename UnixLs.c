#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/dir.h>
#include <stdlib.h>
#include "dirent.h"
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>
#define _XOPEN_SOURCE 700 /* POSIX 2008 plus ... */

#define S _IFMT 0160000 // type of file:
#define S_IDCHR 0020000 // character special
#define MAX_PATH 1024

#ifndef DIRSIZ
#define DIRSIZ 14
#endif

enum commands
{
    i,
    R,
    l
};

struct doCmds
{
    bool i;
    bool l;
    bool R;
};

bool multiPath = false;

struct doCmds doCommands;

void ls_func(char *path)
{
    DIR *d = opendir(path);

    if (d)
    {
        struct dirent *dir;

        struct passwd *pw = NULL;
        struct group *grp;

        char date[200];
        struct tm *t = NULL;

        struct timespec *ts = NULL;

        int width = 7;

        int dfd = dirfd(d);

        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] != '.')
            {
                struct stat stbuf;

                if (fstatat(dfd, dir->d_name, &stbuf, 0) == -1)
                {
                    fprintf(stderr, "fstatat(\"%s/%s\") failed (%d: %s)\n",
                            path, dir->d_name, errno, strerror(errno));
                }
                else
                {
                    if (doCommands.i)
                    {
                        printf("%ld ", stbuf.st_ino);
                    }
                    if (doCommands.l)
                    {
                        printf((S_ISDIR(stbuf.st_mode)) ? "d" : "-");  // file type
                        printf((stbuf.st_mode & S_IRUSR) ? "r" : "-"); // write permissions
                        printf((stbuf.st_mode & S_IWUSR) ? "w" : "-");
                        printf((stbuf.st_mode & S_IXUSR) ? "x" : "-");
                        printf((stbuf.st_mode & S_IRGRP) ? "r" : "-");
                        printf((stbuf.st_mode & S_IWGRP) ? "w" : "-");
                        printf((stbuf.st_mode & S_IXGRP) ? "x" : "-");
                        printf((stbuf.st_mode & S_IROTH) ? "r" : "-");
                        printf((stbuf.st_mode & S_IWOTH) ? "w" : "-");
                        printf((stbuf.st_mode & S_IXOTH) ? "x" : "-");
                        printf(" %ld ", stbuf.st_nlink); // # of hard links
                        pw = getpwuid(stbuf.st_uid);
                        printf(" %s ", pw->pw_name); // file owner
                        grp = getgrgid(stbuf.st_gid);
                        printf(" %s ", grp->gr_name);            // file group
                        printf(" %-*ld ", width, stbuf.st_size); // file size

                        // date and time
                        t = localtime(&stbuf.st_ctim.tv_sec);
                        ts = &stbuf.st_ctim;
                        if (ts->tv_sec != 0)
                        {
                            strftime(date, sizeof(date), "%b %d %Y %H:%M", t);
                            printf("%s ", date);
                        }
                    }
                    printf("%s ", dir->d_name);
                    printf("\n");
                }
            }
        }
        closedir(d);
    }
}

void list(char *);

bool validPath(char *path);

int count = 10;

/**
 * Generate a recursive list of the contents of all subdirectories in a folder
 * command: ls -R
*/
void r_func(char *);

int main(int argc, char **argv)
{

    int index = 0;
    char *cmd = NULL;

    doCommands.i = false;
    doCommands.l = false;
    doCommands.R = false;

    char *path = ".";

    if (argc == 1)
        ls_func("."); // no argument given -> just list files in current directory
    else
    {
        // do we have more than one path
        for (int i = 2; i < argc; i++)
        {
            if (argv[i][0] == '/')
            {
                multiPath = true;
            }
        }
        while (argv[++index] && index <= argc)
        {
            cmd = argv[index];
            if (cmd[0] == '-') // check if argument/ cmd[0] == ‘-‘ is command or path
            {

                for (int i = 1; i < strlen(cmd); i++) //check what commands were given
                {
                    if (cmd[i] != 'i' && cmd[i] != 'R' && cmd[i] != 'l')
                    {
                        printf("UnixLs: invalid option -- '%c': No such file or directory\n", cmd[i]);
                        printf("Try 'UnixLs --help' for more information.\n");
                        return 1;
                    }
                    if (cmd[i] == 'i')
                    {
                        doCommands.i = true;
                    }
                    if (cmd[i] == 'l')
                    {
                        doCommands.l = true;
                    }
                    if (cmd[i] == 'R')
                    {
                        doCommands.R = true;
                    }
                }
            }
            else if (cmd[0] == '/' || cmd[0] == '.' || (cmd[0] == '.' && (cmd[1] == '/' || cmd[1] == '.'))) // if argument given is path
            {
                if (cmd[0] == '.')
                {
                    if (cmd[1] == '.')
                    {
                        list(argv[index]);
                    }
                    else
                    {
                        for (int i = 1; i < strlen(cmd); i++)
                        {
                            if (cmd[i] != '/')
                            {
                                printf("UnixLs: cannot access '%s': No such file or directory\n", cmd);
                                return 1;
                            }
                        }
                        list(argv[index]);
                    }
                }
                else
                {
                    while (argv[index])
                    {
                        if (validPath(argv[index]))
                        {
                            list(argv[index]);
                        }
                        else
                        {
                            printf("UnixLs: cannot access '%s': No such file or directory\n", argv[index]);
                            return 1;
                        }
                        index++;
                    }
                    return 1;
                }
            }
            else
            {
                printf("UnixLs: cannot access '%s': No such file or directory\n", cmd);
                return 1;
            }
        }
        if (index < argc)
        {
            list(path);
        }
    }
    return 0;
}

bool validPath(char *path)
{

    if (strlen(path) == 1)
    {
        if (path[0] != '.' && path[0] != '/') // || strcmp(path, "/") !m= 0)
        {
            return false;
        }
    }
    else
    {
        if (path[0] == '.' || path[0] == '/')
        {
            if (path[0] == '/')
            {
                if (path[1] == '/')
                {
                    for (int i = 1; i < strlen(path); i++)
                    {
                        if (path[i] != '/')
                        {
                            printf("UnixLs: cannot access '%s': No such file or directory\n", path);
                            return false;
                        }
                    }
                }
            }
            else
            {
                if (path[1] == '.' || path[1] == '/')
                {
                    if (path[2] == '/')
                    {
                        for (int i = 1; i < strlen(path); i++)
                        {
                            if (path[i] != '/')
                            {
                                printf("UnixLs: cannot access '%s': No such file or directory\n", path);
                                return false;
                            }
                        }
                    }
                    return false;
                }
            }
        }
    }
    return true;
}

void list(char *path)
{
    if (multiPath)
    {
        printf("%s:\n", path);
    }
    if (doCommands.R)
    {
        r_func(path);
    }
    else
    {
        ls_func(path);
    }
    if (multiPath)
    {
        printf("\n");
    }
}

void r_func(char *path)
{
    // print files
    ls_func(path);

    printf("\n");

    DIR *d = opendir(path); // create pointer to path directory once more so that we can find folders

    if (d)
    {
        struct dirent *dir;

        int dfd = dirfd(d);

        struct stat stbuf;

        char *direct;

        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] != '.')
            {
                if (fstatat(dfd, dir->d_name, &stbuf, 0) == -1)
                {
                    fprintf(stderr, "fstatat(\"%s/%s\") failed (%d: %s)\n",
                            path, dir->d_name, errno, strerror(errno));
                }
                else
                {
                    if ((stbuf.st_mode & S_IFMT) == S_IFDIR) // if file is folder
                    {
                        char buf[1024];

                        if (strcmp(path, ".") == 0)
                        {
                            sprintf(buf, "%s%s/", "./", dir->d_name);
                        }
                        else if (strcmp(path, "..") == 0)
                        {
                            sprintf(buf, "%s%s/", "../", dir->d_name);
                        }
                        else
                        {
                            sprintf(buf, "%s%s/", path, dir->d_name);
                        }
                        printf("%s:\n", buf);
                        r_func(buf);
                    }
                }
            }
        }
    }
    else
    {
        printf("UnixLs: cannot access: '%s': %s\n", path, strerror(errno));
    }
}