#include <gtest/gtest.h>
#include <flinter/types/atomic.h>
#include <flinter/thread/fixed_thread_pool.h>
#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/runnable.h>

class Adder : public flinter::Runnable {
public:
    Adder() : _a(0), _b(0) {}
    virtual ~Adder() {}
    flinter::uatomic64_t _a;
    uint64_t _b;

protected:
    virtual bool Run()
    {
        for (int i = 0; i < 1000000; ++i) {
            _a.AddAndFetch(1);
            ++_b;
        }
        return true;
    }

}; // class Adder

TEST(AtomicTest, TestIncrease)
{
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    flinter::FixedThreadPool pool;
    ASSERT_TRUE(pool.Initialize(10));
    Adder adder;
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(pool.AppendJob(&adder));
    }

    msleep(100);
    pool.Shutdown();
    EXPECT_EQ(adder._a.Get(), 10000000u);
    printf("%lu: %lu\n", adder._a.Get(), adder._b);
}

TEST(AtomicTest, TestCAS)
{
    flinter::atomic64_t a;
    ASSERT_EQ(a.CompareAndSwap(0, 1), 0);
    ASSERT_EQ(a.CompareAndSwap(0, 1), 1);
    ASSERT_EQ(a.CompareAndSwap(0, 1), 1);
}
