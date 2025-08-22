#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <libgen.h>
#include "argumentParser.h"

static int basename_matches(const char *path, const char *pattern) {
    if (pattern == NULL) return 1;
    char *copy = strdup(path);
    if (!copy) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    char *base = basename(copy);
    int rc = fnmatch(pattern, base ? base : "", FNM_PERIOD);
    free(copy);
    return rc == 0;
}

static void creeper(char *path, int depth, int maxdepth, char *name, char type) {
    struct stat sb;
    if (lstat(path, &sb) == -1) {
        perror(path);
        return;
    }

    if (S_ISDIR(sb.st_mode)) {
        if ((type == 'd' || type == 0) && basename_matches(path, name)) {
            if (printf("%s\n", path) < 0) {
                perror("printf");
                exit(EXIT_FAILURE);
            }
        }

        if (maxdepth == -1 || depth < maxdepth) {
            DIR *dirp = opendir(path);
            if (dirp == NULL) {
                perror(path);
                return;
            }

            errno = 0;
            struct dirent *dp;
            while ((dp = readdir(dirp)) != NULL) {
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                    continue;
                }

                size_t newPathLen = strlen(path) + strlen(dp->d_name) + 2;
                char *newPath = malloc(newPathLen);
                if (newPath == NULL) {
                    perror("malloc");
                    closedir(dirp);
                    exit(EXIT_FAILURE);
                }
                int n = snprintf(newPath, newPathLen, "%s/%s", path, dp->d_name);
                if (n < 0 || (size_t)n >= newPathLen) {
                    perror("snprintf");
                    free(newPath);
                    closedir(dirp);
                    exit(EXIT_FAILURE);
                }

                creeper(newPath, depth + 1, maxdepth, name, type);
                free(newPath);
            }
            if (errno != 0) {
                perror("readdir");
                closedir(dirp);
                exit(EXIT_FAILURE);
            }
            if (closedir(dirp) == -1) {
                perror("closedir");
                exit(EXIT_FAILURE);
            }
        }
    } else if (S_ISREG(sb.st_mode)) {
        if ((type == 'f' || type == 0) && basename_matches(path, name)) {
            if (printf("%s\n", path) < 0) {
                perror("printf");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (initArgumentParser(argc, argv) == -1) {
        perror("initArgumentParser");
        exit(EXIT_FAILURE);
    }
    if (getNumberOfArguments() < 1) {
        fprintf(stderr, "Usage: %s path... [-maxdepth=n] [-name=pattern] [-type={d,f}]\n", getCommand());
        exit(EXIT_SUCCESS);
    }

    int maxdepth = -1;
    char *name = NULL;
    char type = 0;

    char *maxdepthStr = getValueForOption("maxdepth");
    if (maxdepthStr != NULL) {
        errno = 0;
        char *end = NULL;
        long v = strtol(maxdepthStr, &end, 10);
        if (errno != 0 || end == maxdepthStr || *end != '\0' || v < 0 || v > INT_MAX) {
            fprintf(stderr, "-maxdepth must be a non-negative integer\n");
            exit(EXIT_FAILURE);
        }
        maxdepth = (int)v;
    }

    name = getValueForOption("name");

    char *typeStr = getValueForOption("type");
    if (typeStr != NULL) {
        if (strcmp(typeStr, "d") == 0) {
            type = 'd';
        } else if (strcmp(typeStr, "f") == 0) {
            type = 'f';
        } else {
            fprintf(stderr, "-type argument must be d or f\n");
            exit(EXIT_FAILURE);
        }
    }

    int nArgs = getNumberOfArguments();
    for (int i = 0; i < nArgs; i++) {
        creeper(getArgument(i), 0, maxdepth, name, type);
    }

    if (fflush(stdout) == EOF) {
        perror("fflush");
        exit(EXIT_FAILURE);
    }

    return 0;
}
