/* Copyright 2016 yiyuanzhong@gmail.com (Yiyuan Zhong)
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

#include "flinter/types/decimal.h"

#include <assert.h>
#include <gmp.h>
#include <stdint.h>
#include <string.h>

#include <sstream>
#include <stdexcept>

namespace flinter {
namespace {
static int IncreaseByOne(mpz_t &q)
{
    if (mpz_sgn(q) > 0) {
        mpz_add_ui(q, q, 1);
        return 1;
    } else {
        mpz_sub_ui(q, q, 1);
        return -1;
    }
}
} // anonymous namespace

class Decimal::Context {
public:
    mpz_t _v;
    int _scale;

}; // class Decimal::Context

Decimal::Decimal() : _context(new Context)
{
    mpz_init(_context->_v);
    _context->_scale = 0;
}

Decimal::~Decimal()
{
    mpz_clear(_context->_v);
    delete _context;
}

Decimal::Decimal(const Decimal &o) : _context(new Context)
{
    mpz_init_set(_context->_v, o._context->_v);
    _context->_scale = o._context->_scale;
}

Decimal::Decimal(const std::string &s) : _context(new Context)
{
    mpz_init(_context->_v);
    if (!Parse(s)) {
        throw std::invalid_argument("invalid decimal string");
    }
}

Decimal::Decimal(const char *s) : _context(new Context)
{
    mpz_init(_context->_v);
    if (!Parse(s)) {
        throw std::invalid_argument("invalid decimal string");
    }
}

#define D(t) \
Decimal::Decimal(unsigned t s) : _context(new Context) \
{ \
    if (sizeof(unsigned t) <= sizeof(unsigned long)) { \
        mpz_init_set_ui(_context->_v, static_cast<unsigned long>(s)); \
    } else { \
        std::ostringstream o; \
        o << s; \
        mpz_init_set_str(_context->_v, o.str().c_str(), 10); \
    } \
    _context->_scale = 0; \
} \
Decimal::Decimal(t s) : _context(new Context) \
{ \
    if (sizeof(t) <= sizeof(long)) { \
        mpz_init_set_si(_context->_v, static_cast<long>(s)); \
    } else { \
        std::ostringstream o; \
        o << s; \
        mpz_init_set_str(_context->_v, o.str().c_str(), 10); \
    } \
    _context->_scale = 0; \
}

D(long long);
D(short);
D(long);
D(char);
D(int);

bool Decimal::Parse(const std::string &s)
{
    char buffer[1024];
    if (s.empty() || s.length() >= sizeof(buffer)) {
        return false;
    }

    int digits = 0;
    int scale = -1;
    size_t pos = 0;
    int zdigits = 0;
    std::string::const_iterator p = s.begin();
    if (*p == '-') {
        buffer[pos++] = *p++;
    }

    for (; p != s.end(); ++p) {
        if (*p >= '0' && *p <= '9') {
            ++digits;
            buffer[pos++] = *p;
            if (scale >= 0) {
                ++scale;
            }

            zdigits = *p == '0' ? zdigits + 1 : 0;
            continue;

        } else if (*p == '.') {
            if (scale >= 0) {
                return false;
            }

            scale = 0;

        } else {
            return false;
        }
    }

    if (!digits) {
        return false;
    }

    if (zdigits == digits) {
        mpz_set_ui(_context->_v, 0);
        _context->_scale = 0;
        return true;
    }

    if (scale > 0) {
        zdigits = std::min(zdigits, scale);
        if (zdigits > 0) {
            scale -= zdigits;
            pos -= static_cast<size_t>(zdigits);
        }
    }

    buffer[pos] = '\0';
    if (mpz_set_str(_context->_v, buffer, 10)) {
        return false;
    }

    scale = std::max(0, scale);
    _context->_scale = scale;
    return true;
}

std::string Decimal::str(int scale) const
{
    std::string s;
    Serialize(&s, scale);
    return s;
}

int Decimal::Serialize(std::string *s, int scale) const
{
    if (scale < 0 && mpz_sgn(_context->_v) == 0) {
        PrintZero(s, 0);
        return 0;
    }

    const int ns = scale >= 0 ? scale : _context->_scale;
    const int extra = ns - _context->_scale;
    if (extra >= 0) {
        return SerializeWithAppending(s, ns);
    } else {
        return SerializeWithRounding(s, ns);
    }
}

int Decimal::SerializeWithTruncating(std::string *s, int scale) const
{
    assert(scale >= 0);
    const int extra = scale - _context->_scale;
    assert(extra < 0);

    size_t len = mpz_sizeinbase(_context->_v, 10);
    if (static_cast<size_t>(-extra) >= len) {
        PrintZero(s, scale);
        return -mpz_sgn(_context->_v);
    }

    s->resize(len + 2);
    if (!mpz_get_str(&(*s)[0], 10, _context->_v)) {
        throw std::runtime_error("failed to call mpz_get_str()");
    }

    len = strlen(s->c_str());
    len -= static_cast<size_t>(-extra);

    bool all_zero = true;
    for (const char *p = s->c_str() + len; *p; ++p) {
        if (*p != '0') {
            all_zero = false;
            break;
        }
    }

    s->resize(len);
    std::string::iterator p = s->begin();
    if (*p == '-') {
        ++p;
        --len;
    }

    int cscale = _context->_scale - -extra;
    if (cscale > 0) {
        if (static_cast<size_t>(cscale) >= len) {
            size_t more = static_cast<size_t>(cscale) - len;
            s->insert(p, more + 2, '0');
            // Now p is gone, calculate again.
            p = s->begin();
            if (*p == '-') {
                *(p + 2) = '.';
            } else {
                *(p + 1) = '.';
            }

        } else {
            ssize_t pos = static_cast<ssize_t>(len) - cscale;
            s->insert(p + pos, '.');
        }
    }

    if (all_zero) {
        return 0;
    } else {
        return -mpz_sgn(_context->_v);
    }
}

int Decimal::SerializeWithAppending(std::string *s, int scale) const
{
    assert(scale >= 0);
    const int extra = scale - _context->_scale;
    assert(extra >= 0);

    s->resize(mpz_sizeinbase(_context->_v, 10) + 2);
    if (!mpz_get_str(&(*s)[0], 10, _context->_v)) {
        throw std::runtime_error("failed to call mpz_get_str()");
    }

    s->resize(strlen(s->c_str()));
    size_t len = s->length();
    std::string::iterator p = s->begin();
    if (*p == '-') {
        ++p;
        --len;
    }

    if (_context->_scale) {
        if (static_cast<size_t>(_context->_scale) >= len) {
            size_t more = static_cast<size_t>(_context->_scale) - len;
            s->insert(p, more + 2, '0');
            // Now p is gone, calculate again.
            p = s->begin();
            if (*p == '-') {
                *(p + 2) = '.';
            } else {
                *(p + 1) = '.';
            }

        } else {
            ssize_t pos = static_cast<ssize_t>(len) - _context->_scale;
            s->insert(p + pos, '.');
        }

    } else if (extra > 0) {
        s->append(".");
    }

    for (int i = 0; i < extra; ++i) {
        s->append("0");
    }

    return 0;
}

int Decimal::SerializeWithRounding(std::string *s, int scale) const
{
    assert(scale >= 0);
    const int extra = scale - _context->_scale;
    assert(extra < 0);

    s->resize(mpz_sizeinbase(_context->_v, 10) + 2);
    if (!mpz_get_str(&(*s)[0], 10, _context->_v)) {
        throw std::runtime_error("failed to call mpz_get_str()");
    }

    s->resize(strlen(s->c_str()));
    size_t len = s->length();
    char *tail = &(*s)[0];
    bool neg = false;

    if (*s->begin() == '-') {
        neg = true;
        ++tail;
        --len;
    }

    if (static_cast<size_t>(-extra) > len) {
        PrintZero(s, scale);
        return -mpz_sgn(_context->_v);
    } else if (static_cast<size_t>(-extra) == len) {
        int ret = TestRoundingTiesToEven('0', tail);
        if (ret <= 0) {
            PrintZero(s, scale);
            return -mpz_sgn(_context->_v);
        } else {
            PrintOne(s, neg, scale);
            return mpz_sgn(_context->_v);
        }
    }

    tail += len - static_cast<size_t>(-extra);
    int ret = TestRoundingTiesToEven(*(tail - 1), tail);
    if (ret <= 0) {
        SerializeWithTruncating(s, scale);
        return -mpz_sgn(_context->_v);

    } else {
        Decimal t(*this);
        mpz_t r;
        mpz_init(r);
        mpz_ui_pow_ui(r, 10, static_cast<unsigned long>(-extra));
        if (neg) {
            mpz_sub(t._context->_v, t._context->_v, r);
        } else {
            mpz_add(t._context->_v, t._context->_v, r);
        }

        mpz_clear(r);
        t.SerializeWithTruncating(s, scale);
        return mpz_sgn(_context->_v);
    }
}

void Decimal::PrintZero(std::string *s, int scale)
{
    if (scale == 0) {
        s->assign("0");
    } else {
        s->assign(static_cast<size_t>(scale) + 2, '0');
        (*s)[1] = '.';
    }
}

void Decimal::PrintOne(std::string *s, bool negative, int scale)
{
    if (scale == 0) {
        s->assign(negative ? "-1" : "1");

    } else if (negative) {
        s->assign(static_cast<size_t>(scale) + 3, '0');
        (*s)[static_cast<size_t>(scale) + 2] = '1';
        (*s)[2] = '.';
        (*s)[0] = '-';

    } else {
        s->assign(static_cast<size_t>(scale) + 2, '0');
        (*s)[static_cast<size_t>(scale) + 1] = '1';
        (*s)[1] = '.';
    }
}

int Decimal::TestRoundingTiesToEven(char last, const char *tail)
{
    if (*tail == '\0') {
        return 0;
    } else if (*tail > '5') {
        return 1;
    } else if (*tail > '0' && *tail < '5') {
        return -1;
    }

    bool all_zero = true;
    for (const char *p = tail + 1; *p; ++p) {
        if (*p != '0') {
            all_zero = false;
            break;
        }
    }

    if (*tail == '0') {
        return all_zero ? 0 : -1;
    } else {
        return all_zero ? ((last - '0') % 2 == 0 ? -1 : 1) : 1;
    }
}

void Decimal::Add(const Decimal &o)
{
    if (_context->_scale < o._context->_scale) {
        Upscale(o._context->_scale - _context->_scale);
        mpz_add(_context->_v, _context->_v, o._context->_v);

    } else if (_context->_scale > o._context->_scale) {
        Decimal t(o);
        t.Upscale(_context->_scale - o._context->_scale);
        mpz_add(_context->_v, _context->_v, t._context->_v);

    } else {
        mpz_add(_context->_v, _context->_v, o._context->_v);
    }

    Cleanup();
}

void Decimal::Sub(const Decimal &o)
{
    if (_context->_scale < o._context->_scale) {
        Upscale(o._context->_scale - _context->_scale);
        mpz_sub(_context->_v, _context->_v, o._context->_v);

    } else if (_context->_scale > o._context->_scale) {
        Decimal t(o);
        t.Upscale(_context->_scale - o._context->_scale);
        mpz_sub(_context->_v, _context->_v, t._context->_v);

    } else {
        mpz_sub(_context->_v, _context->_v, o._context->_v);
    }

    Cleanup();
}

void Decimal::Mul(const Decimal &o)
{
    mpz_mul(_context->_v, _context->_v, o._context->_v);
    _context->_scale += o._context->_scale;
    Cleanup();
}

int Decimal::Div(const Decimal &o, int scale)
{
    if (o.zero()) {
        throw std::invalid_argument("devided by zero");
    } else if (this == &o) {
        mpz_set_ui(_context->_v, 1);
        _context->_scale = 0;
        return 0;
    }

    mpz_t d;
    mpz_t q;
    mpz_t r;
    mpz_init(q);
    mpz_init(r);
    mpz_init_set(d, o._context->_v);

    const int diff = _context->_scale - o._context->_scale;
    if (diff < scale) {
        mpz_ui_pow_ui(q, 10, static_cast<unsigned long>(scale - diff));
        mpz_mul(r, _context->_v, q);
        mpz_tdiv_qr(q, r, r, d);

    } else if (diff > scale) {
        mpz_ui_pow_ui(q, 10, static_cast<unsigned long>(diff - scale));
        mpz_mul(d, d, q);
        mpz_tdiv_qr(q, r, _context->_v, d);

    } else {
        mpz_tdiv_qr(q, r, _context->_v, d);
    }

    int result = 0;
    if (mpz_sgn(r)) { // We've got reminder.
        result = mpz_sgn(q) < 0 ? 1 : -1;
        mpz_abs(r, r);
        mpz_abs(d, d);
        mpz_mul_2exp(r, r, 1); // r <<= 1;

        int cmp = mpz_cmp(r, d);
        if (cmp == 0) { // Oops, in the middle.
            mpz_mod_ui(r, q, 2);
            if (mpz_sgn(r)) { // Odd.
                result = IncreaseByOne(q);
            }

        } else if (cmp > 0) {
            result = IncreaseByOne(q);
        }
    }

    mpz_set(_context->_v, q);
    _context->_scale = scale;

    mpz_clear(r);
    mpz_clear(q);
    mpz_clear(d);
    Cleanup();
    return result;
}

void Decimal::Upscale(int s)
{
    mpz_t t;
    mpz_init(t);
    mpz_ui_pow_ui(t, 10, static_cast<unsigned long>(s));
    mpz_mul(_context->_v, _context->_v, t);
    mpz_clear(t);

    _context->_scale += s;
}

void Decimal::Cleanup()
{
    mpz_t r;
    mpz_init(r);
    while (_context->_scale) {
        mpz_mod_ui(r, _context->_v, 10);
        if (mpz_sgn(r)) {
            break;
        }

        mpz_divexact_ui(_context->_v, _context->_v, 10);
        --_context->_scale;
    }

    mpz_clear(r);
}

bool Decimal::zero() const
{
    return mpz_sgn(_context->_v) == 0;
}

bool Decimal::positive() const
{
    return mpz_sgn(_context->_v) > 0;
}

bool Decimal::negative() const
{
    return mpz_sgn(_context->_v) < 0;
}

bool Decimal::operator > (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) > 0;
}

bool Decimal::operator < (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) < 0;
}

bool Decimal::operator == (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) == 0;
}

bool Decimal::operator != (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) != 0;
}

bool Decimal::operator >= (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) >= 0;
}

bool Decimal::operator <= (const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v) <= 0;
}

int Decimal::compare(const Decimal &o) const
{
    return mpz_cmp(_context->_v, o._context->_v);
}

Decimal &Decimal::operator += (const Decimal &o)
{
    Add(o);
    return *this;
}

Decimal &Decimal::operator -= (const Decimal &o)
{
    Sub(o);
    return *this;
}

Decimal &Decimal::operator *= (const Decimal &o)
{
    Mul(o);
    return *this;
}

Decimal &Decimal::operator /= (const Decimal &o)
{
    Div(o, std::max(_context->_scale, o._context->_scale));
    return *this;
}

Decimal Decimal::operator + (const Decimal &o) const
{
    Decimal n(*this);
    n.Add(o);
    return n;
}

Decimal Decimal::operator - (const Decimal &o) const
{
    Decimal n(*this);
    n.Sub(o);
    return n;
}

Decimal Decimal::operator - () const
{
    Decimal n(*this);
    mpz_neg(n._context->_v, n._context->_v);
    return n;
}

Decimal Decimal::operator * (const Decimal &o) const
{
    Decimal n(*this);
    n.Mul(o);
    return n;
}

Decimal Decimal::operator / (const Decimal &o) const
{
    Decimal n(*this);
    n.Div(o, std::max(_context->_scale, o._context->_scale));
    return n;
}

int Decimal::scale() const
{
    return _context->_scale;
}

} // namespace flinter
