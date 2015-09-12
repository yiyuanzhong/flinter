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

#include "flinter/explode.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>

namespace flinter {

void explode(const std::string &methods,
             const char *delim,
             std::vector<std::string> *result)
{
    result->clear();
    char *buffer = new char[methods.length() + 1];
    memcpy(buffer, methods.c_str(), methods.length() + 1);

    char *ptr = NULL;
    char *token = strtok_r(buffer, delim, &ptr);
    while (token) {
        result->push_back(token);
        token = strtok_r(NULL, delim, &ptr);
    }

    delete [] buffer;
}

void explode(const std::string &methods,
             const char *delim,
             std::list<std::string> *result)
{
    result->clear();
    char *buffer = new char[methods.length() + 1];
    memcpy(buffer, methods.c_str(), methods.length() + 1);

    char *ptr = NULL;
    char *token = strtok_r(buffer, delim, &ptr);
    while (token) {
        result->push_back(token);
        token = strtok_r(NULL, delim, &ptr);
    }

    delete [] buffer;
}

void explode(const std::string &methods,
             const char *delim,
             std::set<std::string> *result)
{
    result->clear();
    char *buffer = new char[methods.length() + 1];
    memcpy(buffer, methods.c_str(), methods.length() + 1);

    char *ptr = NULL;
    char *token = strtok_r(buffer, delim, &ptr);
    while (token) {
        result->insert(token);
        token = strtok_r(NULL, delim, &ptr);
    }

    delete [] buffer;
}

} // namespace flinter
