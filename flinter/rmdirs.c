/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flinter/rmdirs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int rmdirs_internal(const char *path, int leave_dir_along)
{
    size_t pathlen = strlen(path);
    size_t buflen = pathlen + 1 + NAME_MAX + 1;
    char *buffer = NULL;
    struct stat sbuf;
    int ret;

    if (lstat(path, &sbuf)) {
        if (errno == ENOENT) {
            return 0;
        }
        return -1;
    }

    if (!S_ISDIR(sbuf.st_mode)) {
        if (leave_dir_along) {
            return 0;
        }

        return unlink(path);
    }

    buffer = (char *)malloc(buflen);
    if (!buffer) {
        errno = ENOMEM;
        return -1;
    }

    memcpy(buffer, path, pathlen);
    buffer[pathlen++] = '/';

#ifdef __linux__

#ifndef O_DIRECTORY
#define O_DIRECTORY 0200000
#endif

    // On linux, there is a kernel bug that will lead inode numbers in tmpfs
    // overflowed to 0, which cause readdir_r() skip that file. And then, rmdirs
    // will failed since the directory is not empty. So here I use syscall
    // getdents() to avoid this bug.
    struct linux_dirent {
        long           d_ino;
        off_t          d_off;
        unsigned short d_reclen;
        char           d_name[];
    };

    struct linux_dirent *d = NULL;
    char dbuf[4096] = {0};
    int dbuf_used = 0;
    int dbuf_pos = 0;
    int dirfd = -1;

    dirfd = open(path, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        free(buffer);
        return -1;
    }

    for (ret = -1;;) {
        dbuf_used = syscall(SYS_getdents, dirfd, dbuf, sizeof dbuf);
        if (dbuf_used <= 0) {
            ret = 0;
            break;
        }

        for (dbuf_pos = 0; dbuf_pos < dbuf_used;) {
            d = (struct linux_dirent *) (dbuf + dbuf_pos);
            dbuf_pos += d->d_reclen;

            if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")){
                continue;
            }

            strcpy(buffer + pathlen, d->d_name);
            if (rmdirs_internal(buffer, 0) < 0) {
                goto for_end;
            }
        }
    }

for_end:
    close(dirfd);
#else
    struct dirent dirent;
    struct dirent *p;
    DIR *dir;

    dir = opendir(path);
    if (!dir) {
        free(buffer);
        return -1;
    }

    p = NULL;
    ret = -1;

    for (;;) {
        readdir_r(dir, &dirent, &p);
        if (!p) {
            ret = 0;
            break;
        }

        if (memcmp(dirent.d_name, ".", 2) == 0 ||
            memcmp(dirent.d_name, "..", 3) == 0){

            continue;
        }

        strcpy(buffer + pathlen, dirent.d_name);
        if (rmdirs_internal(buffer, 0)) {
            break;
        }
    }

    closedir(dir);
#endif

    free(buffer);

    if (ret) {
        return -1;
    }

    if (leave_dir_along) {
        return 0;
    }

    return rmdir(path);
}

int rmdirs(const char *pathname)
{
    return rmdirs_internal(pathname, 0);
}

int rmdirs_inside(const char *pathname)
{
    return rmdirs_internal(pathname, 1);
}
