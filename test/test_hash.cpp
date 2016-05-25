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

#include "flinter/hash.h"

TEST(hashTest, TestMD5)
{
    char buffer[1024];
    ASSERT_EQ(hash_md5("../test/hash.txt", 1048576, 5000, buffer), 0);
    ASSERT_STREQ(buffer, "ed076287532e86365e841e92bfc50d8c");
}

TEST(hashTest, TestSHA1)
{
    char buffer[1024];
    ASSERT_EQ(hash_sha1("../test/hash.txt", 1048576, 5000, buffer), 0);
    ASSERT_STREQ(buffer, "2ef7bde608ce5404e97d5f042f95f89f1c232871");
}

TEST(hashTest, TestSHA224)
{
    char buffer[1024];
    ASSERT_EQ(hash_sha224("../test/hash.txt", 1048576, 5000, buffer), 0);
    ASSERT_STREQ(buffer, "4575bb4ec129df6380cedde6d71217fe0536f8ffc4e18bca530a7a1b");
}

TEST(hashTest, TestSHA256)
{
    char buffer[1024];
    ASSERT_EQ(hash_sha256("../test/hash.txt", 1048576, 5000, buffer), 0);
    ASSERT_STREQ(buffer, "7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069");
}

TEST(hashTest, TestMurmurhash3)
{
    EXPECT_EQ(hash_murmurhash3("hello", 5), 0x3FA68FD9);
}
