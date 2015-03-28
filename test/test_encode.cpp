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

#include <gtest/gtest.h>

#include "flinter/encode.h"

TEST(encodeTest, TestHex)
{
    static const unsigned char raw[] = { 0x00, 0x01, 0x05, 0x7f, 0x80, 0xff, 0x01 };
    std::string input(raw, raw + sizeof(raw));
    std::string encoded = flinter::EncodeHex(input);
    ASSERT_EQ(encoded, "0001057F80FF01");

    std::string enc = flinter::DecodeHex(encoded);
    ASSERT_EQ(enc.length(), sizeof(raw));
    ASSERT_EQ(memcmp(enc.data(), raw, sizeof(raw)), 0);
}
