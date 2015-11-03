#include <flinter/explode.h>

#include <gtest/gtest.h>

#include <list>
#include <string>
#include <vector>

TEST(ExplodeTest, TestExplodeNull0)
{
    std::vector<std::string> r;
    flinter::explode("", "|", &r, true);
    ASSERT_EQ(r.size(), 1u);
    EXPECT_TRUE(r[0].empty());
}

TEST(ExplodeTest, TestExplodeNull1)
{
    std::vector<std::string> r;
    flinter::explode(",", ",", &r, true);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_TRUE(r[0].empty());
    EXPECT_TRUE(r[1].empty());
}

TEST(ExplodeTest, TestExplodeNull2)
{
    std::vector<std::string> r;
    flinter::explode("||5", "|", &r, true);
    ASSERT_EQ(r.size(), 3u);
    EXPECT_TRUE(r[0].empty());
    EXPECT_TRUE(r[1].empty());
    EXPECT_EQ(r[2], "5");
}

TEST(ExplodeTest, TestExplodeNull3)
{
    std::vector<std::string> r;
    flinter::explode("||6|5,|3|7|||||", "|,", &r, true);
    ASSERT_EQ(r.size(), 12u);
    EXPECT_TRUE(r[0].empty());
    EXPECT_TRUE(r[1].empty());
    EXPECT_EQ(r[2], "6");
    EXPECT_EQ(r[3], "5");
    EXPECT_TRUE(r[4].empty());
    EXPECT_EQ(r[5], "3");
    EXPECT_EQ(r[6], "7");
    EXPECT_TRUE(r[7].empty());
    EXPECT_TRUE(r[8].empty());
    EXPECT_TRUE(r[9].empty());
    EXPECT_TRUE(r[10].empty());
    EXPECT_TRUE(r[11].empty());
}

TEST(ExplodeTest, TestExplodeNull4)
{
    std::vector<std::string> r;
    flinter::explode("6|5||3|7", "||", &r, true);
    ASSERT_EQ(r.size(), 5u);
    EXPECT_EQ(r[0], "6");
    EXPECT_EQ(r[1], "5");
    EXPECT_TRUE(r[2].empty());
    EXPECT_EQ(r[3], "3");
    EXPECT_EQ(r[4], "7");
}

TEST(ExplodeTest, TestExplodeNotNull0)
{
    std::list<std::string> s;
    flinter::explode("", "|", &s, false);
    ASSERT_EQ(s.size(), 0u);
}

TEST(ExplodeTest, TestExplodeNotNull1)
{
    std::list<std::string> s;
    flinter::explode(",", ",", &s, false);
    ASSERT_EQ(s.size(), 0u);
}

TEST(ExplodeTest, TestExplodeNotNull2)
{
    std::list<std::string> s;
    flinter::explode("||5", "|", &s, false);
    ASSERT_EQ(s.size(), 1u);
    EXPECT_EQ(s.front(), "5");
}

TEST(ExplodeTest, TestExplodeNotNull3)
{
    std::vector<std::string> r;
    flinter::explode("||6|5,|3|7|||||", ",|", &r, false);
    ASSERT_EQ(r.size(), 4u);
    EXPECT_EQ(r[0], "6");
    EXPECT_EQ(r[1], "5");
    EXPECT_EQ(r[2], "3");
    EXPECT_EQ(r[3], "7");
}

TEST(ExplodeTest, TestExplodeNotNull4)
{
    std::vector<std::string> r;
    flinter::explode("6|5||3|7", "||", &r, false);
    ASSERT_EQ(r.size(), 4u);
    EXPECT_EQ(r[0], "6");
    EXPECT_EQ(r[1], "5");
    EXPECT_EQ(r[2], "3");
    EXPECT_EQ(r[3], "7");
}

TEST(ExplodeTest, TestExplodeSet)
{
    std::set<std::string> r;
    flinter::explode("6|5||3|7", "||", &r);
    ASSERT_EQ(r.size(), 4u);
    std::set<std::string>::iterator p = r.begin();
    EXPECT_EQ(*p++, "3");
    EXPECT_EQ(*p++, "5");
    EXPECT_EQ(*p++, "6");
    EXPECT_EQ(*p++, "7");
}

TEST(ExplodeTest, TestImplode)
{
    static const char *a[] = { "hello", "world", "abc" };
    std::vector<std::string> b(a, a + 3);
    std::string o;
    flinter::implode("__", a, a + 3, &o);
    EXPECT_EQ(o, "hello__world__abc");
    flinter::implode(",", b, &o);
    EXPECT_EQ(o, "hello,world,abc");
    flinter::implode("__", a, a + 1, &o);
    EXPECT_EQ(o, "hello");
    flinter::implode("__", a, a, &o);
    EXPECT_EQ(o, "");
    b.clear();
    flinter::implode(",", b, &o);
    EXPECT_EQ(o, "");
    b.push_back("aaa");
    flinter::implode(",", b, &o);
    EXPECT_EQ(o, "aaa");
}
