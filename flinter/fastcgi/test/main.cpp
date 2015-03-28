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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "flinter/fastcgi/dispatcher.h"

static void print_version(const char *argv0)
{
    printf("OSS CGI 1.0.0.0\n");
}

static void print_help(const char *argv0)
{
    printf("Usage: %s [OPTION]...\n\n", argv0);
    printf("  -v, --version  display version and exit.\n");
    printf("  -h, --help     display usage and exit.\n");
    printf("  -f, --config   use specified configure file.\n");
}

int main(int argc, char *argv[])
{
    static const struct option longopts[] = {
        { "help",             no_argument, NULL, 'h' },
        { "version",          no_argument, NULL, 'v' },
        { "config",     required_argument, NULL, 'f' },
        { NULL,                         0, NULL,  0  },
    };

    int opt;
    optind = 0;
    while ((opt = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;
        case 'v':
            print_version(argv[0]);
            return EXIT_SUCCESS;
        case '?':
            print_help(argv[0]);
            return EXIT_FAILURE;
        default:
            break;
        };
    }

    return flinter::Dispatcher::main(argc, argv);
}
