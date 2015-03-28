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
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int rmdirs_internal(const char *pathname, int leave_dir_along)
{
    struct dirent dirent;
    struct dirent *p;
    struct stat buf;
    char *buffer;
    size_t len;
    DIR *dir;
    int ret;

    if (lstat(pathname, &buf)) {
        if (errno == ENOENT) {
            return 0;
        }
        return -1;
    }

    if (!S_ISDIR(buf.st_mode)) {
        if (leave_dir_along) {
            return 0;
        }

        return unlink(pathname);
    }

    len = strlen(pathname);
    buffer = (char *)malloc(len + 1 + NAME_MAX + 1);
    if (!buffer) {
        errno = ENOMEM;
        return -1;
    }

    memcpy(buffer, pathname, len);
    buffer[len++] = '/';

    dir = opendir(pathname);
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

        strcpy(buffer + len, dirent.d_name);
        if (rmdirs_internal(buffer, 0)) {
            break;
        }
    }

    closedir(dir);
    free(buffer);

    if (ret) {
        return -1;
    }

    if (leave_dir_along) {
        return 0;
    }

    return rmdir(pathname);
}

int rmdirs(const char *pathname)
{
    return rmdirs_internal(pathname, 0);
}

int rmdirs_inside(const char *pathname)
{
    return rmdirs_internal(pathname, 1);
}
