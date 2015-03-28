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

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include "flinter/convert.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>

#include <algorithm>

namespace flinter {

#define DEFINE_S(type,name,min,max) \
type name(const std::string &from, type defval) { \
    if (from.empty()) { return defval; } \
    const char *v = from.c_str(); \
    char *p; \
    errno = 0; \
    long long r = strtoll(v, &p, 0); \
    if (errno || p != v + from.length()) { return defval; } \
    if (r < (min) || r > (max)) { return defval; } \
    return static_cast<type>(r); \
}

#define DEFINE_U(type,name,max) \
type name(const std::string &from, type defval) { \
    if (from.empty()) { return defval; } \
    const char *v = from.c_str(); \
    char *p; \
    errno = 0; \
    unsigned long long r = strtoull(v, &p, 0); \
    if (errno || p != v + from.length()) { return defval; } \
    if (r > (max)) { return defval; } \
    return static_cast<type>(r); \
}

#define DEFINE_D(type,name,min,max) \
type name(const std::string &from, type defval) { \
    if (from.empty()) { return defval; } \
    const char *v = from.c_str(); \
    char *p; \
    errno = 0; \
    long double r = strtold(v, &p); \
    if (errno || p != v + from.length()) { return defval; } \
    if (r < (min) || r > (max)) { return defval; } \
    return static_cast<type>(r); \
}

#define DEFINE_TO_S(type,name,min,max) DEFINE_S(type,convert_to_##name,min,max)
#define DEFINE_TO_U(type,name,max)     DEFINE_U(type,convert_to_##name,max)
#define DEFINE_TO_D(type,name,min,max) DEFINE_D(type,convert_to_##name,min,max)

DEFINE_TO_S(  int8_t,   int8,   INT8_MIN,  INT8_MAX);
DEFINE_TO_S( int16_t,  int16,  INT16_MIN, INT16_MAX);
DEFINE_TO_S( int32_t,  int32,  INT32_MIN, INT32_MAX);
DEFINE_TO_S( int64_t,  int64,  INT64_MIN, INT64_MAX);
DEFINE_TO_U( uint8_t,  uint8,  UINT8_MAX           );
DEFINE_TO_U(uint16_t, uint16, UINT16_MAX           );
DEFINE_TO_U(uint32_t, uint32, UINT32_MAX           );
DEFINE_TO_U(uint64_t, uint64, UINT64_MAX           );

DEFINE_TO_S(         char     ,   char,  SCHAR_MIN, SCHAR_MAX);
DEFINE_TO_S(         short    ,  short,   SHRT_MIN,  SHRT_MAX);
DEFINE_TO_S(         int      ,    int,    INT_MIN,   INT_MAX);
DEFINE_TO_S(         long     ,   long,   LONG_MIN,  LONG_MAX);
DEFINE_TO_S(         long long,  llong,  LLONG_MIN, LLONG_MAX);
DEFINE_TO_U(unsigned char     ,  uchar,  UCHAR_MAX           );
DEFINE_TO_U(unsigned short    , ushort,  USHRT_MAX           );
DEFINE_TO_U(unsigned int      ,   uint,   UINT_MAX           );
DEFINE_TO_U(unsigned long     ,  ulong,  ULONG_MAX           );
DEFINE_TO_U(unsigned long long, ullong, ULLONG_MAX           );

DEFINE_TO_D(float      , float  ,  FLT_MIN,  FLT_MAX);
DEFINE_TO_D(double     , double ,  DBL_MIN,  DBL_MAX);
DEFINE_TO_D(long double, ldouble, LDBL_MIN, LDBL_MAX);

} // namespace flinter
