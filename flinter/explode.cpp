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
namespace {

template <class T>
void explode_internal(const std::string &methods,
                      const char *delim,
                      bool preserve_null,
                      T *result)
{
    result->clear();
    size_t pos = 0;
    while (pos < methods.length()) {
        size_t hit = methods.find_first_of(delim, pos);
        if (hit == std::string::npos) {
            result->push_back(methods.substr(pos));
            return;
        }

        if (pos != hit || preserve_null) {
            result->push_back(methods.substr(pos, hit - pos));
        }

        pos = hit + 1;
    }

    if (preserve_null) {
        result->push_back(std::string());
    }
}

} // anonymous namespace

void explode(const std::string &methods,
             const char *delim,
             std::vector<std::string> *result,
             bool preserve_null)
{
    explode_internal(methods, delim, preserve_null, result);
}

void explode(const std::string &methods,
             const char *delim,
             std::list<std::string> *result,
             bool preserve_null)
{
    explode_internal(methods, delim, preserve_null, result);
}

void explode(const std::string &methods,
             const char *delim,
             std::set<std::string> *result)
{
    result->clear();
    size_t pos = 0;
    while (pos < methods.length()) {
        size_t hit = methods.find_first_of(delim, pos);
        if (hit == std::string::npos) {
            result->insert(methods.substr(pos));
            break;
        }

        if (pos != hit) {
            result->insert(methods.substr(pos, hit - pos));
        }

        pos = hit + 1;
    }
}

} // namespace flinter
