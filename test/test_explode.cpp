#include <flinter/explode.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(ImplodeTest, TestImplode)
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
