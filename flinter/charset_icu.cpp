/* Copyright 2015 yiyuanzhong@gmail.com (Yiyuan Zhong)
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

#include <unicode/ucnv.h>

#include "flinter/types/auto_buffer.h"

namespace flinter {
namespace {

int charset_icu_load(const std::string &input,
                     flinter::AutoBuffer<UChar> *output,
                     const char *encoding,
                     size_t *length)
{
    UConverter *conv;
    UErrorCode error;

    error = U_ZERO_ERROR;
    conv = ucnv_open(encoding, &error);
    if (!conv) {
        return -1;
    }

    error = U_ZERO_ERROR;
    ucnv_setToUCallBack(conv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    if (error) {
        return -1;
    }

    long outpos = 0;
    size_t outlen = 1024;
    const char *source = input.c_str();
    const char *const sourceLimit = source + input.length();

    while (true) {
        output->resize(outlen);
        UChar *target = output->get() + outpos;
        UChar *const targetLimit = output->get() + outlen;
        outlen *= 2;

        error = U_ZERO_ERROR;
        ucnv_toUnicode(conv,
                       &target, targetLimit,
                       &source, sourceLimit,
                       NULL, TRUE, &error);

        outpos = target - output->get();
        if (error == U_ZERO_ERROR) {
            *length = static_cast<size_t>(outpos);
            break;
        } else if (error != U_BUFFER_OVERFLOW_ERROR) {
            ucnv_close(conv);
            return -1;
        }
    }

    ucnv_close(conv);
    return 0;
}

int charset_icu_save(const flinter::AutoBuffer<UChar> &input,
                     std::string *output,
                     const char *encoding,
                     size_t length)
{
    UConverter *conv;
    UErrorCode error;

    error = U_ZERO_ERROR;
    conv = ucnv_open(encoding, &error);
    if (!conv) {
        return -1;
    }

    error = U_ZERO_ERROR;
    ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    if (error) {
        return -1;
    }

    long outpos = 0;
    size_t outlen = 1024;
    const UChar *source = input.get();
    const UChar *const sourceLimit = source + length;

    while (true) {
        output->resize(outlen);
        char *const out = &(*output)[0];
        char *target = out + outpos;
        char *const targetLimit = out + outlen;
        outlen *= 2;

        error = U_ZERO_ERROR;
        ucnv_fromUnicode(conv,
                         &target, targetLimit,
                         &source, sourceLimit,
                         NULL, TRUE, &error);

        outpos = target - out;
        if (error == U_ZERO_ERROR) {
            output->resize(static_cast<size_t>(outpos));
            break;
        } else if (error != U_BUFFER_OVERFLOW_ERROR) {
            ucnv_close(conv);
            return -1;
        }
    }

    ucnv_close(conv);
    return 0;
}

int charset_icu(const std::string &input,
                std::string *output,
                const char *from,
                const char *to)
{
    assert(from && *from);
    assert(to && *to);

    if (!output) {
        return -1;
    } else if (input.empty()) {
        output->clear();
        return 0;
    }

    flinter::AutoBuffer<UChar> i;
    size_t length;
    int ret;

    ret = charset_icu_load(input, &i, from, &length);
    if (ret) {
        return ret;
    }

    ret = charset_icu_save(i, output, to, length);
    if (ret) {
        return ret;
    }

    return 0;
}

} // anonymous namespace

int charset_utf8_to_gbk(const std::string &utf, std::string *gbk)
{
    return charset_icu(utf, gbk, "UTF-8", "GBK");
}

int charset_gbk_to_utf8(const std::string &gbk, std::string *utf)
{
    return charset_icu(gbk, utf, "GBK", "UTF-8");
}

int charset_utf8_to_gb18030(const std::string &utf, std::string *gbk)
{
    return charset_icu(utf, gbk, "UTF-8", "GB18030");
}

int charset_gb18030_to_utf8(const std::string &gbk, std::string *utf)
{
    return charset_icu(gbk, utf, "GB18030", "UTF-8");
}

} // namespace flinter
