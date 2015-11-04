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

#ifndef __FLINTER_CHARSET_H__
#define __FLINTER_CHARSET_H__

#include <stdint.h>

#include <string>
#include <vector>

namespace flinter {

/*
 * UTF-8 and GB18030 is recommended encoding for Unicode and Chinese.
 *
 * CP (code point) in charset library is acting as an internal encoding, it's
 * in platform endian thus NOT suitable on wire. Please understand this is
 * the only encoding that is of fixed width for any code points.
 *
 * If you can, avoid GBK.
 */

extern int charset_utf8_to_gbk(const std::string &utf, std::string *gbk);
extern int charset_gbk_to_utf8(const std::string &gbk, std::string *utf);

extern int charset_utf8_to_gb18030(const std::string &utf, std::string *gbk);
extern int charset_gb18030_to_utf8(const std::string &gbk, std::string *utf);

extern int charset_cp_to_gbk(const std::string &cp, std::string *gbk);
extern int charset_gbk_to_cp(const std::string &gbk, std::string *cp);

extern int charset_utf8_to_cp(const std::string &utf, std::vector<int32_t> *cp);
extern int charset_cp_to_utf8(const std::vector<int32_t> &cp, std::string *utf);

extern int charset_utf8_to_json(const std::string &utf, std::string *json);

extern int charset_gbk_to_wide(const std::string &gbk, std::wstring *wide)  __attribute__ ((deprecated));
extern int charset_wide_to_gbk(const std::wstring &wide, std::string *gbk)  __attribute__ ((deprecated));
extern int charset_utf8_to_wide(const std::string &utf, std::wstring *wide) __attribute__ ((deprecated));
extern int charset_wide_to_utf8(const std::wstring &wide, std::string *utf) __attribute__ ((deprecated));

} // namespace flinter

#endif /* __FLINTER_CHARSET_H__ */
