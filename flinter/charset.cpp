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
#include <stdio.h>

namespace flinter {

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
