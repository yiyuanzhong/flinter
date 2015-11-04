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

#include "flinter/charset.h"

#include <assert.h>
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>

namespace flinter {
namespace {

template <class F, class T>
static int charset_iconv(const F &input,
                         T *output,
                         const char *from,
                         const char *to)
{
    if (!output) {
        return -1;
    }

    output->clear();
    if (input.empty()) {
        return 0;
    }

    iconv_t cd = iconv_open(to, from);
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return -1;
    }

    char buffer[1024];
    char *outbuf = buffer;
    size_t outbytesleft = sizeof(buffer);
    size_t inbytesleft = input.size() * sizeof(typename F::value_type);
    char *inbuf = const_cast<char *>(reinterpret_cast<const char *>(&input[0]));

    int result = -1;
    while (true) {
        size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        if (ret == static_cast<size_t>(-1)) {
            if (errno == EILSEQ) { // Invalid character encountered.
                break;
            } else if (errno == EINVAL) { // Input is incomplete.
                break;
            } else if (errno == E2BIG) { // Recoverable.
                output->insert(output->end(),
                               reinterpret_cast<typename T::value_type *>(buffer),
                               reinterpret_cast<typename T::value_type *>(outbuf));

                outbytesleft = sizeof(buffer);
                outbuf = buffer;
                continue;

            } else { // Undocumented return value.
                break;
            }

        } else if (ret) {
            break;
        }

        output->insert(output->end(),
                       reinterpret_cast<typename T::value_type *>(buffer),
                       reinterpret_cast<typename T::value_type *>(outbuf));

        result = 0;
        break;
    }

    iconv_close(cd);
    return result;
}

} // anonymous namespace

int charset_utf8_to_wide(const std::string &utf, std::wstring *wide)
{
    return charset_iconv(utf, wide, "UTF-8", "WCHAR_T");
}

int charset_wide_to_utf8(const std::wstring &wide, std::string *utf)
{
    return charset_iconv(wide, utf, "WCHAR_T", "UTF-8");
}

int charset_gbk_to_wide(const std::string &gbk, std::wstring *wide)
{
    return charset_iconv(gbk, wide, "GBK", "WCHAR_T");
}

int charset_wide_to_gbk(const std::wstring &wide, std::string *gbk)
{
    return charset_iconv(wide, gbk, "WCHAR_T", "GBK");
}

#define CHARSET(f,t,ef,et) \
int charset_##f##_to_##t(const std::string &from, std::string *to) \
{ \
    return charset_iconv(from, to, ef, et); \
}

#define CHARSET_I(f,t,ef,et) \
int charset_##f##_to_##t(const std::vector<int32_t> &from, std::string *to) \
{ \
    return charset_iconv(from, to, ef, et); \
}

#define CHARSET_O(f,t,ef,et) \
int charset_##f##_to_##t(const std::string &from, std::vector<int32_t> *to) \
{ \
    return charset_iconv(from, to, ef, et); \
}

CHARSET(utf8,    gbk,     "UTF-8",          "GBK");
CHARSET(gbk,     utf8,    "GBK",            "UTF-8");
CHARSET(utf8,    gb18030, "UTF-8",          "GB18030");
CHARSET(gb18030, utf8,    "GB18030",        "UTF-8");

CHARSET_O(utf8,  cp,      "UTF-8",          "UCS-4-INTERNAL");
CHARSET_I(cp,    utf8,    "UCS-4-INTERNAL", "UTF-8");
CHARSET_O(gbk,   cp,      "GBK",            "UCS-4-INTERNAL");
CHARSET_I(cp,    gbk,     "UCS-4-INTERNAL", "GBK");

} // namespace flinter
