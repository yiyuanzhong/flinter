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

#include "flinter/signals.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

static int signals_do_mask_some(int mask, int signum, va_list ap)
{
    sigset_t set;
    int sig;

    if (sigemptyset(&set)) {
        return -1;
    }

    for (sig = signum; sig; sig = va_arg(ap, int)) {
        if (sigaddset(&set, sig)) {
            return -1;
        }
    }

    if (sigprocmask(mask, &set, NULL)) {
        return -1;
    }

    return 0;
}

static int signals_do_mask_except(int mask, int signum, va_list ap)
{
    sigset_t set;
    int sig;

    if (sigfillset(&set)) {
        return -1;
    }

    for (sig = signum; sig; sig = va_arg(ap, int)) {
        if (sigdelset(&set, sig)) {
            return -1;
        }
    }

    if (sigprocmask(mask, &set, NULL)) {
        return -1;
    }

    return 0;
}

static int signals_do_set_some(void (*handler)(int), int signum, va_list ap)
{
    int sig;

    for (sig = signum; sig; sig = va_arg(ap, int)) {
        if (signals_set_handler(sig, handler)) {
            if (errno != EINVAL) {
                return -1;
            }
        }
    }

    return 0;
}

static int signals_do_set_except(void (*handler)(int), int signum, va_list ap)
{
    sigset_t set;
    int sig;

    if (sigemptyset(&set)) {
        return -1;
    }

    for (sig = signum; sig; sig = va_arg(ap, int)) {
        if (sigaddset(&set, sig)) {
            return -1;
        }
    }

    for (sig = 1; sig < NSIG; ++sig) {
        if (!sigismember(&set, sig)) {
            if (signals_set_handler(sig, handler)) {
                if (errno != EINVAL) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

static int signals_set_all_except(void (*handler)(int), int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_set_except(handler, signum, ap);
    va_end(ap);

    return ret;
}

int signals_ignore_all(void)
{
    return signals_set_all_except(SIG_IGN, 0);
}

int signals_default_all(void)
{
    return signals_set_all_except(SIG_DFL, 0);
}

int signals_unblock_all(void)
{
    return signals_unblock_all_except(0);
}

int signals_block_all(void)
{
    return signals_block_all_except(0);
}

int signals_block_some(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_mask_some(SIG_BLOCK, signum, ap);
    va_end(ap);

    return ret;
}

int signals_unblock_some(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_mask_some(SIG_UNBLOCK, signum, ap);
    va_end(ap);

    return ret;
}

int signals_ignore_some(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_set_some(SIG_IGN, signum, ap);
    va_end(ap);

    return ret;
}

int signals_default_some(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_set_some(SIG_DFL, signum, ap);
    va_end(ap);

    return ret;
}

int signals_block_all_except(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_mask_except(SIG_BLOCK, signum, ap);
    va_end(ap);

    return ret;
}

int signals_unblock_all_except(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_mask_except(SIG_UNBLOCK, signum, ap);
    va_end(ap);

    return ret;
}

int signals_ignore_all_except(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_set_except(SIG_IGN, signum, ap);
    va_end(ap);

    return ret;
}

int signals_default_all_except(int signum, ...)
{
    va_list ap;
    int ret;

    va_start(ap, signum);
    ret = signals_do_set_except(SIG_DFL, signum, ap);
    va_end(ap);

    return ret;
}

int signals_block(int signum)
{
    return signals_block_some(signum, 0);
}

int signals_unblock(int signum)
{
    return signals_unblock_some(signum, 0);
}

int signals_ignore(int signum)
{
    return signals_set_handler(signum, SIG_IGN);
}

int signals_default(int signum)
{
    return signals_set_handler(signum, SIG_DFL);
}

int signals_set_handler(int signum, void (*handler)(int))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    if (sigemptyset(&sa.sa_mask)) {
        return -1;
    }

    sa.sa_handler = handler;
    return sigaction(signum, &sa, NULL);
}

int signals_set_action(int signum, void (*action)(int, siginfo_t *, void *))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    if (sigemptyset(&sa.sa_mask)) {
        return -1;
    }

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = action;
    return sigaction(signum, &sa, NULL);
}
