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

#include "flinter/utility.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(WIN32)
#include <Windows.h>
#include <WinCrypt.h>
#pragma comment (lib, "Advapi32.lib")
#elif defined(__unix__)
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "flinter/msleep.h"
#include "flinter/safeio.h"
#include "config.h"

extern char *environ[];
#endif /* __unix__ */

static int make_clean_path_stage_one(const char *path, char **stage, int trim)
{
    const char *p;
    size_t len;
    char *r;

    /* Stage 1.1, allocate memory. */
    len = strlen(path);

    /* I only need 2 + 1 bytes to store "./", but trailing unaligned bytes could trigger
     * memory detectors (like valgrind) to report false positive errors. */
    len += 7;
    *stage = (char *)malloc(len);
    if (!*stage) {
        errno = ENOMEM;
        return -1;
    }

    /* Stage 1.2, ltrim. */
    p = path;
    if (trim) {
        for (; *p; ++p) {
            if (*p != ' ' && *p != '\t') {
                break;
            }
        }
    }

    /* Stage 1.3, remove duplicated slashes and replace backslashes. */
    r = *stage;
    for (; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            if (r == *stage || *(r - 1) != '/') {
                *r++ = '/';
            }
            continue;
        }

        *r++ = *p;
    }

    /* Stage 1.4, rtrim. */
    if (trim) {
        for (; r != *stage; --r) {
            if (*(r - 1) != ' ' && *(r - 1) != '\t') {
                break;
            }
        }
    }

    /* Stage 1.5, fix blank input. */
    if (r == *stage) {
        *r++ = '.';
    }

    /* Stage 1.6, add trailing slashes. */
    if (*(r - 1) != '/') {
        *r++ = '/';
    }

    /* Stage 1.7, close string. */
    *r = '\0';
    return 0;
}

static int make_clean_path_stage_two(const char *stage, char **result)
{
    const char *p;
    const char *q;
    size_t len;
    char *r;
    char *s;

    /* Stage 2.1, allocate memory. */
    len = strlen(stage);
    *result = malloc(len + 2 + 1);
    if (!*result) {
        errno = ENOMEM;
        return -1;
    }

    /* Stage 2.2, parse stage input. */
    r = *result;
    p = stage;
    if (*p == '/') {
        ++p;
        *r++ = '/';
    }

    for (; *p; ++p) {
        for (q = p; *q != '/'; ++q) {
            /* Nothing but to skip a token. */
        }

        /* I hate this part. */
        if (*p == '.') {
            if (q - p == 2 && *(p + 1) == '.') {
                p += 2;
                if (r == *result) {
                    *r++ = '.';
                    *r++ = '.';
                    *r++ = '/';
                    continue;
                }

                for (s = r - 1; s != *result && *(s - 1) != '/'; --s) {
                    /* Nothing but to rewind a token. */
                }

                if (s == *result) {
                    if (*s == '/') { /* /.. is still / */
                        continue;

                    } else if (*s != '.') {
                        r = s;
                        continue;
                    }
                }

                if (r - s == 3 && *s == '.' && *(s + 1) == '.') {
                    *r++ = '.';
                    *r++ = '.';
                    *r++ = '/';
                    continue;
                }

                r = s;
                continue;

            } else if (q - p == 1) {
                ++p;
                continue;

            } else {
                memcpy(r, p, q - p);
                r += q - p;
            }

            *r++ = '/';

        } else {
            memcpy(r, p, q - p + 1);
            r += q - p + 1;
            p += q - p;
        }
    }

    /* Stage 2.3, fix blank input. */
    if (r == *result) {
        *r++ = '.';
    }

    /* Stage 2.4, remove trailing slashes. */
    if (*(r - 1) == '/') {
        if (r - 1 != *result) {
            --r;
        }
    }

    /* Stage 2.5, close string. */
    *r = '\0';
    return 0;
}

char *make_clean_path(const char *path, int trim)
{
    char *result;
    char *stage;

    if (!path) {
        errno = EINVAL;
        return NULL;
    }

    /* Pass 1, allocate buffer to store normalized input. */
    if (make_clean_path_stage_one(path, &stage, trim)) {
        return NULL;
    }

    /* Pass 2, deal with "." and "..". */
    if (make_clean_path_stage_two(stage, &result)) {
        free(stage);
        return NULL;
    }

    free(stage);
    return result;
}

int32_t atoi32(const char *value)
{
    char *end;
    long i;

    if (!value || !*value) {
        return -1;
    }

    i = strtol(value, &end, 10);
    if (end != value + strlen(value)        ||
        (i == LONG_MAX && errno == ERANGE)  ||
        i > INT32_MAX                       ||
        i < 0                               ){

        return -1;
    }

    return (int32_t)i;
}

#if defined(WIN32)
void randomize(void)
{
    unsigned int seed;
    int64_t timestamp;
    HCRYPTPROV hProv;

    if (CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, 0)) {
        if (CryptGenRandom(hProv, sizeof(seed), (BYTE *)&seed)) {
            CryptReleaseContext(hProv, 0);
            srand(seed);
            return;
        }
        CryptReleaseContext(hProv, 0);
    }

    /* Not good, try time based random initializer. */
    timestamp = get_wall_clock_timestamp();
    if (timestamp >= 0) {
        seed = (unsigned int)(timestamp ^ GetProcessId(GetCurrentProcess()));
        srand(seed);
        return;
    }

    /* What the hell? */
    srand((unsigned int)GetProcessId(GetCurrentProcess()));
}

int64_t get_wall_clock_timestamp(void)
{
    ULARGE_INTEGER uli;
    int64_t result;
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);
    uli.HighPart = ft.dwHighDateTime;
    uli.LowPart = ft.dwLowDateTime;
    result = (int64_t)((uli.QuadPart - 116444736000000000ULL) * 100);
    return result;
}

int64_t get_monotonic_timestamp(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    double temp;

    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&counter)     ){

        return get_wall_clock_timestamp();
    }

    temp = (double)counter.QuadPart;
    temp /= frequency.QuadPart;
    temp *= 1000000000;
    return (int64_t)temp;
}

time_t get_monotonic_time(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    double temp;

    if (!QueryPerformanceFrequency(&frequency) ||
        !QueryPerformanceCounter(&counter)     ){

        return get_wall_clock_timestamp();
    }

    temp = (double)counter.QuadPart;
    temp /= frequency.QuadPart;
    return (time_t)temp;
}
#elif defined(__unix__)
int add_to_set(int max_fd, int fd, fd_set *set)
{
    assert(set);
    if (fd < 0 || fd >= (int)FD_SETSIZE) {
        return max_fd;
    }

    FD_SET(fd, set);
    if (max_fd < fd) {
        max_fd = fd;
    }

    return max_fd;
}

int set_non_blocking_mode(int fd)
{
    int options = fcntl(fd, F_GETFL);
    if (options < 0) {
        return -1;
    }

    if ((options & O_NONBLOCK) == O_NONBLOCK) {
        return 0;
    }

    options |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, options)) {
        return -1;
    }

    return 0;
}

int set_blocking_mode(int fd)
{
    int options = fcntl(fd, F_GETFL);
    if (options < 0) {
        return -1;
    }

    if ((options & O_NONBLOCK) != O_NONBLOCK) {
        return 0;
    }

    options &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, options)) {
        return -1;
    }

    return 0;
}

int set_socket_address_reuse(int sockfd)
{
    static const int one = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, (socklen_t)sizeof(one));
}

int set_alarm_timer(int milliseconds)
{
    struct itimerval timer;
    if (milliseconds < 0) {
        milliseconds = 0;
    }

    timer.it_value.tv_sec = milliseconds / 1000;
    timer.it_value.tv_usec = milliseconds % 1000 * 1000;
    timer.it_interval = timer.it_value;
    if (setitimer(ITIMER_REAL, &timer, NULL)) {
        return -1;
    }

    return 0;
}

int set_maximum_files(int nofile)
{
    struct rlimit rlim;
    struct rlimit old;

    if (nofile < 0) {
        errno = EINVAL;
        return -1;
    }

    if (getrlimit(RLIMIT_NOFILE, &rlim)) {
        return -1;
    }

    if (rlim.rlim_cur == (rlim_t)nofile) {
        return rlim.rlim_cur;
    }

    old = rlim;
    rlim.rlim_cur = nofile;
    if (rlim.rlim_max != RLIM_INFINITY && rlim.rlim_max < (rlim_t)nofile) {
        /* Try to override. */
        rlim.rlim_max = nofile;
        if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) { /* Cool! */
            return rlim.rlim_cur;
        }

        /* No way to get more. */
        if (old.rlim_cur == old.rlim_max) {
            return old.rlim_cur;
        }

        rlim.rlim_cur = rlim.rlim_max = old.rlim_max;
    }

    if (setrlimit(RLIMIT_NOFILE, &rlim)) {
        return old.rlim_cur;
    }

    return rlim.rlim_cur;
}

void randomize(void)
{
    int fd;
    int64_t timestamp;
    unsigned int seed;

    /* Try /dev/urandom first. */
    fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (safe_read(fd, &seed, sizeof(seed)) == sizeof(seed)) {
            safe_close(fd);
            srand(seed);
            return;
        }

        safe_close(fd);
    }

    /* Not good, try time based random initializer. */
    timestamp = get_wall_clock_timestamp();
    if (timestamp >= 0) {
        seed = (unsigned int)(timestamp ^ getpid());
        srand(seed);
        return;
    }

    /* What the hell? */
    srand((unsigned int)getpid());
}

int64_t get_wall_clock_timestamp(void)
{
    time_t t;
    uint64_t result;
    struct timeval tv;

#if HAVE_CLOCK_GETTIME
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
        result = (int64_t)(1000000000LL * tp.tv_sec + tp.tv_nsec);
        return result;
    }
#elif defined(__MACH__)
    clock_serv_t cclock;
    if (host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock) == KERN_SUCCESS) {
        mach_timespec_t mts;
        if (clock_get_time(cclock, &mts) == 0) {
            result = (int64_t)(1000000000LL * mts.tv_sec + mts.tv_nsec);
            mach_port_deallocate(mach_task_self(), cclock);
            return result;
        }
        mach_port_deallocate(mach_task_self(), cclock);
    }
#endif

    if (gettimeofday(&tv, NULL) == 0) {
        result = (int64_t)(1000000000LL * tv.tv_sec + tv.tv_usec * 1000LL);
        return result;
    }

    if ((t = time(NULL)) >= 0) {
        result = (int64_t)(1000000000LL * t);
        return result;
    }

    return -1;
}

int64_t get_monotonic_timestamp(void)
{
#if HAVE_CLOCK_GETTIME
    int64_t result;
    struct timespec tp;
#ifdef CLOCK_MONOTONIC_PRECISE
    if (clock_gettime(CLOCK_MONOTONIC_PRECISE, &tp) == 0) {
        result = (int64_t)(1000000000LL * tp.tv_sec + tp.tv_nsec);
        return result;
    }
#endif

#ifdef CLOCK_MONOTONIC_HR
    if (clock_gettime(CLOCK_MONOTONIC_HR, &tp) == 0) {
        result = (int64_t)(1000000000LL * tp.tv_sec + tp.tv_nsec);
        return result;
    }
#endif

    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
        result = (int64_t)(1000000000LL * tp.tv_sec + tp.tv_nsec);
        return result;
    }
#elif defined(__MACH__)
    clock_serv_t cclock;
    if (host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock) == KERN_SUCCESS) {
        mach_timespec_t mts;
        if (clock_get_time(cclock, &mts) == KERN_SUCCESS) {
            int64_t result = (int64_t)(1000000000LL * mts.tv_sec + mts.tv_nsec);
            mach_port_deallocate(mach_task_self(), cclock);
            return result;
        }
        mach_port_deallocate(mach_task_self(), cclock);
    }
#endif /* HAVE_CLOCK_GETTIME */

    /* Oh my god, use wall clock instead. */
    return get_wall_clock_timestamp();
}

time_t get_monotonic_time(void)
{
#if HAVE_CLOCK_GETTIME
    struct timespec tp;
#ifdef CLOCK_MONOTONIC_PRECISE
    if (clock_gettime(CLOCK_MONOTONIC_PRECISE, &tp) == 0) {
        return tp.tv_sec;
    }
#endif

#ifdef CLOCK_MONOTONIC_HR
    if (clock_gettime(CLOCK_MONOTONIC_HR, &tp) == 0) {
        return tp.tv_sec;
    }
#endif

    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
        return tp.tv_sec;
    }
#elif defined(__MACH__)
    clock_serv_t cclock;
    if (host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock) == KERN_SUCCESS) {
        mach_timespec_t mts;
        if (clock_get_time(cclock, &mts) == KERN_SUCCESS) {
            time_t result = mts.tv_sec;
            mach_port_deallocate(mach_task_self(), cclock);
            return result;
        }
        mach_port_deallocate(mach_task_self(), cclock);
    }
#endif /* HAVE_CLOCK_GETTIME */

    /* Oh my god, use wall clock instead. */
    return time(NULL);
}

int set_close_on_exec(int fd)
{
    int ret;
    ret = fcntl(fd, F_GETFD);
    if (ret < 0) {
        return ret;
    }

    ret |= FD_CLOEXEC;
    return fcntl(fd, F_SETFD, ret);
}
#endif /* __unix__ */
