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

#include <string>

namespace flinter {

extern int charset_utf8_to_gbk(const std::string &utf, std::string *gbk);
extern int charset_gbk_to_utf8(const std::string &gbk, std::string *utf);

extern int charset_gbk_to_wide(const std::string &gbk, std::wstring *wide);
extern int charset_wide_to_gbk(const std::wstring &wide, std::string *gbk);

extern int charset_utf8_to_wide(const std::string &utf, std::wstring *wide);
extern int charset_wide_to_utf8(const std::wstring &wide, std::string *utf);

extern int charset_utf8_to_json(const std::string &utf, std::string *json);

extern int charset_utf8_to_gb18030(const std::string &utf, std::string *gbk);
extern int charset_gb18030_to_utf8(const std::string &gbk, std::string *utf);

} // namespace flinter

#endif /* __FLINTER_CHARSET_H__ */
