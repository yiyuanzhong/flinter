#include <gtest/gtest.h>

#include <flinter/charset.h>

TEST(CharsetTest, TestUtf8AndGb18030)
{
    std::string q;
    std::string o = "emm";
    std::string p = "妈妈说不管多长的字符串flinter都可以转换出来！";
    EXPECT_EQ(0, flinter::charset_utf8_to_gb18030("", &o));
    EXPECT_TRUE(o.empty());
    EXPECT_EQ(0, flinter::charset_utf8_to_gb18030(p, &q));
    EXPECT_EQ(0, flinter::charset_gb18030_to_utf8(q, &o));
    EXPECT_EQ(p, o);
    EXPECT_EQ(p.size(), 64u);
    EXPECT_EQ(q.size(), 45u);
}

TEST(CharsetTest, TestUtf8AndGbk)
{
    std::string q;
    std::string o = "emm";
    std::string p = "妈妈说不管多长的字符串flinter都可以转换出来！";
    EXPECT_EQ(0, flinter::charset_utf8_to_gbk("", &o));
    EXPECT_TRUE(o.empty());
    EXPECT_EQ(0, flinter::charset_utf8_to_gbk(p, &q));
    EXPECT_EQ(0, flinter::charset_gbk_to_utf8(q, &o));
    EXPECT_EQ(p, o);
    EXPECT_EQ(p.size(), 64u);
    EXPECT_EQ(q.size(), 45u);
}

TEST(CharsetTest, TestUtf8AndJson)
{
    std::string p;
    std::string q;
    std::string r;
    p = "a中b\"b文c\tc\3c";
    r = "a\\u4E2Db\\\"b\\u6587c\\tc\\u0003c";
    ASSERT_EQ(0, flinter::charset_utf8_to_json(p, &q));
    EXPECT_EQ(q, r);
}
