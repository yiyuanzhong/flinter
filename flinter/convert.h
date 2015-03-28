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

#ifndef __FLINTER_CONVERT_H__
#define __FLINTER_CONVERT_H__

#include <stdint.h>

#include <sstream>
#include <string>

namespace flinter {

extern   int8_t convert_to_int8  (const std::string &from,   int8_t defval = 0);
extern  int16_t convert_to_int16 (const std::string &from,  int16_t defval = 0);
extern  int32_t convert_to_int32 (const std::string &from,  int32_t defval = 0);
extern  int64_t convert_to_int64 (const std::string &from,  int64_t defval = 0);
extern  uint8_t convert_to_uint8 (const std::string &from,  uint8_t defval = 0);
extern uint16_t convert_to_uint16(const std::string &from, uint16_t defval = 0);
extern uint32_t convert_to_uint32(const std::string &from, uint32_t defval = 0);
extern uint64_t convert_to_uint64(const std::string &from, uint64_t defval = 0);

extern char        convert_to_char (const std::string &from, char      defval = 0);
extern short       convert_to_short(const std::string &from, short     defval = 0);
extern int         convert_to_int  (const std::string &from, int       defval = 0);
extern long        convert_to_long (const std::string &from, long      defval = 0);
extern long long   convert_to_llong(const std::string &from, long long defval = 0);

extern unsigned char      convert_to_uchar (const std::string &from,
                                            unsigned char       defval = 0);

extern unsigned short     convert_to_ushort(const std::string &from,
                                            unsigned short      defval = 0);

extern unsigned int       convert_to_uint  (const std::string &from,
                                            unsigned int        defval = 0);

extern unsigned long      convert_to_ulong (const std::string &from,
                                            unsigned long       defval = 0);

extern unsigned long long convert_to_ullong(const std::string &from,
                                            unsigned long long  defval = 0);

extern float        convert_to_float  (const std::string &from, float       defval = 0.0);
extern double       convert_to_double (const std::string &from, double      defval = 0.0);
extern long double  convert_to_ldouble(const std::string &from, long double defval = 0.0);

} // namespace flinter

#endif // __FLINTER_CONVERT_H__
