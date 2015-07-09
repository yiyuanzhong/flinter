#include <gtest/gtest.h>

#include <flinter/msleep.h>
#include <flinter/timeout_pool.h>

class T {
public:
    explicit T(int *d) : _d(d) {}
    ~T() { ++*_d; }

private:
    int *_d;

};

TEST(TimeoutPoolTest, TestDeconstruct)
{
    flinter::TimeoutPool<int, T *> *t = new flinter::TimeoutPool<int, T *>(50000000LL);
    int d = 0;
    t->Insert(0, new T(&d));
    EXPECT_EQ(d, 0);
    delete t;
    EXPECT_EQ(d, 1);
}

TEST(TimeoutPoolTest, TestAutoRelease)
{
    flinter::TimeoutPool<int, T *> t(50000000LL, true);
    int d = 0;
    t.Insert(0, new T(&d));
    t.Insert(1, new T(&d), 200000000LL);
    EXPECT_EQ(d, 0);
    t.Check();
    EXPECT_EQ(d, 0);
    msleep(100);
    EXPECT_EQ(d, 0);
    t.Check();
    EXPECT_EQ(d, 1);
    msleep(200);
    EXPECT_EQ(d, 1);
    t.Check();
    EXPECT_EQ(d, 2);
}

TEST(TimeoutPoolTest, TestConflict)
{
    flinter::TimeoutPool<int, T *> t(50000000LL, true);
    int d = 0;
    t.Insert(0, new T(&d));
    EXPECT_EQ(d, 0);
    t.Insert(0, new T(&d));
    EXPECT_EQ(d, 1);
    t.Clear();
    EXPECT_EQ(d, 2);
    t.Clear();
    EXPECT_EQ(d, 2);
}
