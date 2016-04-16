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

#ifndef __FLINTER_TYPES_DECIMAL_H__
#define __FLINTER_TYPES_DECIMAL_H__

#include <string>

namespace flinter {

class Decimal {
public:
    Decimal();
    ~Decimal();
    /* explicit */Decimal(const std::string &s);
    /* explicit */Decimal(unsigned long long s);
    /* explicit */Decimal(unsigned short s);
    /* explicit */Decimal(unsigned char s);
    /* explicit */Decimal(unsigned long s);
    /* explicit */Decimal(unsigned int s);
    /* explicit */Decimal(const char *s);
    /* explicit */Decimal(long long s);
    /* explicit */Decimal(short s);
    /* explicit */Decimal(char s);
    /* explicit */Decimal(long s);
    /* explicit */Decimal(int s);

    bool Parse(const std::string &s);
    std::string str(int scale = -1) const;

    /// @retval <0 result is lesser than actual value.
    /// @retval  0 result is exact.
    /// @retval >0 result is greater than actual value.
    int Serialize(std::string *s, int scale) const;

    bool zero() const;
    bool positive() const;
    bool negative() const;

    void Add(const Decimal &o);
    void Sub(const Decimal &o);
    void Mul(const Decimal &o);

    /// @retval <0 result is lesser than actual value.
    /// @retval  0 result is exact.
    /// @retval >0 result is greater than actual value.
    int Div(const Decimal &o, int scale);

    Decimal &operator += (const Decimal &o);
    Decimal &operator -= (const Decimal &o);
    Decimal &operator *= (const Decimal &o);
    Decimal &operator /= (const Decimal &o);

    Decimal operator + (const Decimal &o) const;
    Decimal operator - (const Decimal &o) const;
    Decimal operator * (const Decimal &o) const;
    Decimal operator / (const Decimal &o) const;
    Decimal operator - () const;

    bool operator != (const Decimal &o) const;
    bool operator == (const Decimal &o) const;
    bool operator >= (const Decimal &o) const;
    bool operator <= (const Decimal &o) const;
    bool operator < (const Decimal &o) const;
    bool operator > (const Decimal &o) const;
    Decimal &operator = (const Decimal &o);
    int compare(const Decimal &o) const;
    Decimal(const Decimal &o);
    int scale() const;

protected:
    /// @retval <0 truncate
    /// @retval  0 exact
    /// @retval >0 round
    static int TestRoundingTiesToEven(char last, const char *tail);
    static void PrintOne(std::string *s, bool negative, int scale);
    static void PrintZero(std::string *s, int scale);

    int SerializeWithTruncating(std::string *s, int scale) const;
    int SerializeWithAppending(std::string *s, int scale) const;
    int SerializeWithRounding(std::string *s, int scale) const;
    void Upscale(int s);
    void Cleanup();

private:
    class Context;
    Context *const _context;

}; // class Decimal

} // namespace flinter

#endif // __FLINTER_TYPES_DECIMAL_H__
