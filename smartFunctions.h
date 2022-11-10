#ifndef NSU_OS_LABS_2_SMARTFUNCTIONS_H
#define NSU_OS_LABS_2_SMARTFUNCTIONS_H

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>

#include "defAssigns.h"

#define NOT_READY 1
#define RETRY_TIME_SEC 5

void *smartCalloc(size_t num, size_t size);

int smartOpenFile(char *path, int oflag, mode_t mode);

DIR *smartOpenDirectory(char *path);

int smartCreateThread(void *param, void *(*function)(void *));


#endif //NSU_OS_LABS_2_SMARTFUNCTIONS_H