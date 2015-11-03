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

#ifndef __FLINTER_EXPLODE_H__
#define __FLINTER_EXPLODE_H__

#include <list>
#include <set>
#include <string>
#include <vector>

namespace flinter {

extern void explode(const std::string &methods,
                    const char *delim,
                    std::vector<std::string> *result,
                    bool preserve_null = false);

extern void explode(const std::string &methods,
                    const char *delim,
                    std::list<std::string> *result,
                    bool preserve_null = false);

extern void explode(const std::string &methods,
                    const char *delim,
                    std::set<std::string> *result);

template <class iterator>
inline void implode(const std::string &glue,
                    iterator begin, iterator end,
                    std::string *result)
{
    result->clear();
    if (begin == end) {
        return;
    }

    iterator p = begin;
    result->assign(*p++);
    for (; p != end; ++p) {
        result->append(glue);
        result->append(*p);
    }
}

template <class T>
inline void implode(const std::string &glue,
                    const T &container,
                    std::string *result)
{
    implode(glue, container.begin(), container.end(), result);
}

} // namespace flinter

#endif // __FLINTER_EXPLODE_H__
