#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "defAssigns.h"
#include "smartFunctions.h"

#define DEF_BUFF_SIZE 256
#define MIN_REQ_ARGS_NUM 3

typedef struct PathsInfo {
    char *source;
    char *destination;
} PathsInfo;

extern int errno;

void *copyFiles(void *args);

char *concatStrings(const char **strings) {
    if (IS_PTR_EMPTY(strings)) {
        fprintf(stderr, "concatStrings. incorrect strings");
        return EMPTY;
    }

    size_t summaryLength = 0;
    for (int i = 0; !IS_PTR_EMPTY(strings[i]); ++i) {
        summaryLength += strlen(strings[i]);
    }

    char *concStr = (char *) smartCalloc(1, summaryLength + 1);
    if (IS_PTR_EMPTY(concStr)) {
        return EMPTY;
    }

    for (int i = 0; !IS_PTR_EMPTY(strings[i]); ++i) {
        strcat(concStr, strings[i]);
    }

    return concStr;
}

void destroyPaths(PathsInfo *paths) {
    if (IS_PTR_EMPTY(paths)) {
        return;
    }
    free(paths->destination);
    free(paths->source);
    free(paths);
}

PathsInfo *createPaths(const char *sourcePath, const char *destinationPath, const char *subPath) {
    if (IS_PTR_EMPTY(sourcePath) || IS_PTR_EMPTY(destinationPath)) {
        fprintf(stderr, "createPaths. invalid source or destination path");
    }

    PathsInfo *paths = smartCalloc(1, sizeof(PathsInfo));
    if (IS_PTR_EMPTY(paths)) {
        return EMPTY;
    }

    char *delimiter = IS_STR_EMPTY(subPath) ? "" : "/";
    subPath = IS_STR_EMPTY(subPath) ? "" : subPath;

    const char *fullPathFromSource[] = {sourcePath, delimiter, subPath, NULL};
    const char *fullPathFromDest[] = {destinationPath, delimiter, subPath, NULL};

    paths->source = concatStrings(fullPathFromSource);
    paths->destination = concatStrings(fullPathFromDest);

    if (IS_PTR_EMPTY(paths->source) || IS_PTR_EMPTY(paths->destination)) {
        destroyPaths(paths);
        return EMPTY;
    }

    return paths;
}

int copyFileInternals(PathsInfo *paths, int sourceFd, int destinationFd) {
    if (IS_PTR_EMPTY(paths)) {
        fprintf(stderr, "copyFileInternals. invalid paths\n");
        return STATUS_FAILURE;
    }

    char buf[DEF_BUFF_SIZE];
    while (NOT_READY) {
        ssize_t offset = 0;
        ssize_t bytesWritten;
        ssize_t bytesRead = read(sourceFd, buf, DEF_BUFF_SIZE);
        if (bytesRead == STATUS_FAILURE) {
            fprintf(stderr, "copyFileInternals. There are problems with reading file from %s", paths->source);
            return STATUS_FAILURE;
        }

        if (bytesRead == 0) {
            break;
        }

        while (offset < bytesRead) {
            bytesWritten = write(destinationFd, buf + offset, bytesRead - offset);
            if (bytesWritten == STATUS_FAILURE) {
                fprintf(stderr, "copyFileInternals. There are problems with reading file from %s", paths->destination);
                return STATUS_FAILURE;
            }
            offset += bytesWritten;
        }
    }

    return STATUS_SUCCESS;
}

int traverseDirectory(DIR *dir, struct dirent *entryBuffer, PathsInfo *paths) {
    if (IS_PTR_EMPTY(paths)) {
        fprintf(stderr, "traverseDirectory");
        return STATUS_FAILURE;
    }

    int status;
    struct dirent *resDirent = NULL;

    while (NOT_READY) {
        status = readdir_r(dir, entryBuffer, &resDirent);
        if (status != STATUS_SUCCESS) {
            fprintf(stderr, "traverseDirectory. There are problems with (readdir_r)");
            return STATUS_FAILURE;
        }
        if (IS_PTR_EMPTY(resDirent)) {
            return STATUS_FAILURE;
        }
        if (IS_STRS_EQUAL(entryBuffer->d_name, ".") || IS_STRS_EQUAL(entryBuffer->d_name, "..")) {
            continue;
        }
        PathsInfo *pathsNew = createPaths(paths->source, paths->destination, entryBuffer->d_name);
        if (IS_PTR_EMPTY(pathsNew)) {
            continue;
        }
        status = smartCreateThread((void *) pathsNew, copyFiles);
        if (status != STATUS_SUCCESS) {
            destroyPaths(pathsNew);
            return STATUS_FAILURE;
        }
    }
}

int copyDirectory(PathsInfo *paths, mode_t mode) {
    if (IS_PTR_EMPTY(paths) || IS_PTR_EMPTY(paths->source) || IS_PTR_EMPTY(paths->destination)) {
        fprintf(stderr, "copyDirectory. incorrect paths\n");
        return STATUS_FAILURE;
    }

    if (mkdir(paths->destination, mode) == STATUS_FAILURE && errno != EEXIST && errno != STATUS_SUCCESS) {
        fprintf(stderr, "copyDirectory. problems with make directory (mkdir)\n");
        return STATUS_FAILURE;
    }

    int status;

    DIR *dir = smartOpenDirectory(paths->source);
    if (IS_PTR_EMPTY(dir)) {
        fprintf(stderr, "copyDirectory. problems with opening directory\n");
        return STATUS_FAILURE;
    }

    struct dirent *entry = (struct dirent *) smartCalloc(1,
                                                         sizeof(struct dirent) + pathconf(paths->source, _PC_NAME_MAX) +
                                                         1);
    if (entry == NULL) {
        status = closedir(dir);
        if (status == STATUS_FAILURE) {
            fprintf(stderr, "copyDirectory. problems with closing directory (closedir)\n");
        }
        return STATUS_FAILURE;
    }

    traverseDirectory(dir, entry, paths);

    free(entry);
    status = closedir(dir);
    if (status == STATUS_FAILURE) {
        fprintf(stderr, "copyDirectory. problems with closing directory (closedir)\n");
    }

    return STATUS_SUCCESS;
}

int copyRegularFile(PathsInfo *paths, mode_t mode) {
    if (IS_PTR_EMPTY(paths) || IS_PTR_EMPTY(paths->source) || IS_PTR_EMPTY(paths->destination)) {
        fprintf(stderr, "copyRegularFile. incorrect paths\n");
        return STATUS_FAILURE;
    }

    int status;

    int sourceFd = smartOpenFile(paths->source, O_RDONLY, mode);
    if (sourceFd == STATUS_FAILURE) {
        return STATUS_FAILURE;
    }

    int destinationFd = smartOpenFile(paths->destination, O_WRONLY | O_CREAT, mode);
    if (destinationFd == STATUS_FAILURE) {
        status = close(sourceFd);
        if (status == STATUS_FAILURE) {
            fprintf(stderr, "copyRegularFile. Problems with closing file (close)");
        }
        return STATUS_FAILURE;
    }

    copyFileInternals(paths, sourceFd, destinationFd);

    status = close(sourceFd);
    if (status == STATUS_FAILURE) {
        fprintf(stderr, "copyRegularFile. Problems with closing file on path (close): %s", paths->source);
        return STATUS_FAILURE;
    }

    status = close(destinationFd);
    if (status == STATUS_FAILURE) {
        fprintf(stderr, "copyRegularFile. Problems with closing file on path (close): %s", paths->destination);
        return STATUS_FAILURE;
    }

}

void *copyFiles(void *args) {
    if (IS_PTR_EMPTY(args)) {
        fprintf(stderr, "copyFiles. invalid args");
        return EMPTY;
    }

    PathsInfo *paths = (PathsInfo *) args;

    struct stat statbuffer;

    if (lstat(paths->source, &statbuffer) == STATUS_FAILURE) {
        destroyPaths(paths);
        return EMPTY;
    }

    if (S_ISDIR(statbuffer.st_mode)) {
        copyDirectory(paths, statbuffer.st_mode);
    } else if (S_ISREG(statbuffer.st_mode)) {
        copyRegularFile(paths, statbuffer.st_mode);
    }

    destroyPaths(paths);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != MIN_REQ_ARGS_NUM) {
        fprintf(stderr, "usage: <programExecFile> sourcePath destinationPath\n");
        return EXIT_SUCCESS;
    }

    PathsInfo *paths = createPaths(argv[1], argv[2], "");
    if (IS_PTR_EMPTY(paths)) {
        return EXIT_FAILURE;
    }

    copyFiles(paths);

    pthread_exit(NULL);
}