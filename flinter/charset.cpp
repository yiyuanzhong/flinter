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
#include <stdlib.h>

namespace flinter {
namespace {

template <class T>
static int charset_iconv(const void *input,
                         size_t input_size,
                         std::basic_string<T> *output,
                         const char *from,
                         const char *to)
{
    if (!output) {
        return -1;
    }

    output->clear();
    if (!input_size) {
        return 0;
    }

    iconv_t cd = iconv_open(to, from);
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return -1;
    }

    char buffer[1024];
    char *outbuf = buffer;
    size_t inbytesleft = input_size;
    size_t outbytesleft = sizeof(buffer);
    char *inbuf = reinterpret_cast<char *>(const_cast<void *>(input));

    int result = -1;
    while (true) {
        size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
        if (ret == static_cast<size_t>(-1)) {
            if (errno == EILSEQ) { // Invalid character encountered.
                break;
            } else if (errno == EINVAL) { // Input is incomplete.
                break;
            } else if (errno == E2BIG) { // Recoverable.
                output->append(reinterpret_cast<T *>(buffer),
                               reinterpret_cast<T *>(outbuf));

                outbytesleft = sizeof(buffer);
                outbuf = buffer;
                continue;

            } else { // Undocumented return value.
                break;
            }

        } else if (ret) {
            break;
        }

        output->append(reinterpret_cast<T *>(buffer),
                       reinterpret_cast<T *>(outbuf));

        result = 0;
        break;
    }

    iconv_close(cd);
    return result;
}

} // anonymous namespace

int charset_utf8_to_gbk(const std::string &utf, std::string *gbk)
{
    return charset_iconv(utf.data(), utf.length(), gbk, "UTF-8", "GBK");
}

int charset_gbk_to_utf8(const std::string &gbk, std::string *utf)
{
    return charset_iconv(gbk.data(), gbk.length(), utf, "GBK", "UTF-8");
}

int charset_utf8_to_wide(const std::string &utf, std::wstring *wide)
{
    return charset_iconv(utf.data(), utf.length(), wide, "UTF-8", "WCHAR_T");
}

int charset_wide_to_utf8(const std::wstring &wide, std::string *utf)
{
    return charset_iconv(wide.data(), wide.length() * sizeof(wchar_t), utf,
                         "WCHAR_T", "UTF-8");
}

int charset_gbk_to_wide(const std::string &gbk, std::wstring *wide)
{
    return charset_iconv(gbk.data(), gbk.length(), wide, "GBK", "WCHAR_T");
}

int charset_wide_to_gbk(const std::wstring &wide, std::string *gbk)
{
    return charset_iconv(wide.data(), wide.length() * sizeof(wchar_t), gbk,
                         "WCHAR_T", "GBK");
}

int charset_utf8_to_json(const std::string &utf, std::string *json)
{
    if (!json) {
        return -1;
    } else if (utf.empty()) {
        json->clear();
        return 0;
    }

    json->clear();
    std::wstring wide;
    if (charset_utf8_to_wide(utf, &wide)) {
        return -1;
    }

    char buffer[32];
    for (std::wstring::const_iterator p = wide.begin(); p != wide.end(); ++p) {
        if (*p > 0x1f && *p < 0x7f) {
            buffer[0] = static_cast<char>(*p);
            buffer[1] = '\0';

        } else if (*p <= 0xffff) {
            if (snprintf(buffer, sizeof(buffer), "\\u%04x", *p) != 6) {
                return -1;
            }

        } else {
            return -1;
        }

        json->append(buffer);
    }

    return 0;
}

} // namespace flinter
