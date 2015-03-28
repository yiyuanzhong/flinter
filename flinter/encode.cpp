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

#include "flinter/encode.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sstream>

#include "config.h"
#if HAVE_CLEARSILVER_CLEARSILVER_H
#include <ClearSilver/ClearSilver.h>
#endif

#if HAVE_OPENSSL_OPENSSLV_H
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#endif

namespace flinter {
namespace {

static int HexChar(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return -1;
    }
}

} // anonymous namespace

#if HAVE_CLEARSILVER_CLEARSILVER_H
std::string EncodeUrl(const std::string &url)
{
    char *ret;
    NEOERR *err = cgi_url_escape(url.c_str(), &ret);
    if (err != STATUS_OK) {
        nerr_ignore(&err);
        return std::string();
    }

    std::string result = ret;
    free(ret);
    return result;
}

std::string DecodeUrl(const std::string &url)
{
    std::string copy(url);
    return cgi_url_unescape(&copy[0]);
}
#endif // HAVE_CLEARSILVER_CLEARSILVER_H

int EncodeHex(const std::string &what, std::string *hex)
{
    static const char HEX[] = "0123456789ABCDEF";

    if (what.empty()) {
        hex->clear();
        return 0;
    }

    hex->resize(what.length() * 2);
    std::string::iterator p = hex->begin();
    for (std::string::const_iterator q = what.begin(); q != what.end(); ++q) {
        *p++ = HEX[static_cast<uint8_t>(*q) / 16];
        *p++ = HEX[static_cast<uint8_t>(*q) % 16];
    }

    return 0;
}

std::string EncodeHex(const std::string &what)
{
    std::string hex;
    if (EncodeHex(what, &hex)) {
        return std::string();
    }

    return hex;
}

int DecodeHex(const std::string &hex, std::string *what)
{
    size_t length = hex.length();
    if (!length || (length % 2)) {
        return -1;
    }

    what->resize(length / 2);
    std::string::iterator p = what->begin();
    for (size_t i = 0; i < hex.length(); i += 2) {
        int h = HexChar(hex[i]);
        int l = HexChar(hex[i + 1]);
        if (h < 0 || l < 0) {
            return -1;
        }

        *p++ = static_cast<char>(h * 16 + l);
    }

    return 0;
}

std::string DecodeHex(const std::string &hex)
{
    std::string what;
    if (DecodeHex(hex, &what)) {
        return std::string();
    }

    return what;
}

std::string EscapeHtml(const std::string &html, bool ie_compatible)
{
    if (html.empty()) {
        return html;
    }

    size_t extra = 0;
    for (std::string::const_iterator p = html.begin(); p != html.end(); ++p) {
        switch (*p) {
        case '<' :
        case '>' : extra += 3; break;
        case '&' : extra += 4; break;
        case '\'':
        case '"' : extra += 5; break;
        default  :             break;
        };
    }

    if (!extra) {
        return html;
    }

    std::string result;
    result.reserve(html.length() + extra);
    const char *apos = ie_compatible ? "&#039;" : "&apos;";
    for (std::string::const_iterator p = html.begin(); p != html.end(); ++p) {
        switch (*p) {
        case '<' : result += "&lt;"  ; break;
        case '>' : result += "&gt;"  ; break;
        case '&' : result += "&amp;" ; break;
        case '"' : result += "&quot;"; break;
        case '\'': result += apos    ; break;
        default  : result += *p      ; break;
        };
    }

    return result;
}

#if HAVE_OPENSSL_OPENSSLV_H
int EncodeBase64(const std::string &input, std::string *output)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    if (!output) {
        errno = EINVAL;
        return -1;
    }

    if (input.empty()) {
        output->clear();
        return 0;
    }

    b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        errno = ENOMEM;
        return -1;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    if (!bmem) {
        BIO_free_all(b64);
        errno = ENOMEM;
        return -1;
    }

    b64 = BIO_push(b64, bmem);
    if (BIO_write(b64,
                  reinterpret_cast<const unsigned char *>(input.data()),
                  static_cast<int>(input.length())) <= 0) {

        BIO_free_all(b64);
        errno = EACCES;
        return -1;
    }

    if (BIO_flush(b64) != 1) {
        BIO_free_all(b64);
        errno = EACCES;
        return -1;
    }

    BIO_get_mem_ptr(b64, &bptr);
    output->resize(static_cast<size_t>(bptr->length));
    memcpy(&output->at(0), bptr->data, static_cast<size_t>(bptr->length));
    BIO_free_all(b64);
    return 0;
}

int DecodeBase64(const std::string &input, std::string *output)
{
    BIO *b64, *bmem;
    size_t buflen;
    size_t size;
    int ret;

    if ((input.length() % 4 != 0) || !output) {
        errno = EINVAL;
        return -1;
    }

    if (input.empty()) {
        output->clear();
        return 0;
    }

    buflen = input.length() / 4 * 3;
    output->resize(buflen);

    b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        errno = ENOMEM;
        return -1;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    unsigned char *inbuf = reinterpret_cast<unsigned char *>(const_cast<char *>(input.data()));
    bmem = BIO_new_mem_buf(inbuf, static_cast<int>(input.length()));
    if (!bmem) {
        BIO_free_all(b64);
        errno = ENOMEM;
        return -1;
    }

    bmem = BIO_push(b64, bmem);
    ret = BIO_read(bmem, &output->at(0), static_cast<int>(buflen));
    size = static_cast<size_t>(ret);
    if (size < buflen - 2 || size > buflen) {
        BIO_free_all(bmem);
        errno = EACCES;
        return -1;
    }

    output->resize(size);
    BIO_free_all(bmem);
    return 0;
}
#endif // HAVE_OPENSSL_OPENSSLV_H

} // namespace flinter
